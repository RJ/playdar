#include "darknet.h"
#include "msgs.h"
#include "servent.h"
#include "ss_darknet.h"
#include <iostream>                        
#include <boost/asio.hpp>                  
#include <boost/lexical_cast.hpp>          
#include <boost/thread.hpp>                
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp> 
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cassert>
#include <boost/lexical_cast.hpp>
#include <cassert>

using namespace playdar::resolvers;

bool
darknet::init(playdar::Config * c, Resolver * r)
{
    m_resolver = r;
    m_conf = c;
    unsigned short port = conf()->get<int>("plugins.darknet.port",9999);
    m_io_service = boost::shared_ptr<boost::asio::io_service>
                   (new boost::asio::io_service);
    m_work = boost::shared_ptr<boost::asio::io_service::work>
             (new boost::asio::io_service::work(*m_io_service));
    m_servent = boost::shared_ptr< Servent >
                (new Servent(*m_io_service, port, this));
    // start io_services:
    cout << "Darknet servent coming online on port " <<  port <<endl;
    
    m_threads = new boost::thread_group; // TODO configurable threads?
    for (std::size_t i = 0; i < 5; ++i)
    {
        m_threads->create_thread(boost::bind(
            &boost::asio::io_service::run, m_io_service.get()));
    }
 
    // get peers: TODO support multiple/list from config
    string remote_ip = conf()->get<string>("plugins.darknet.peerip","");
    if(remote_ip!="")
    {
        unsigned short remote_port = conf()->get<int>
                                     ("plugins.darknet.peerport",9999);
        cout << "Attempting peer connect: " 
             << remote_ip << ":" << remote_port << endl;
        boost::asio::ip::address_v4 ipaddr = boost::asio::ip::address_v4::from_string(remote_ip);
        boost::asio::ip::tcp::endpoint ep(ipaddr, remote_port);
        m_servent->connect_to_remote(ep);
    }
    return true;
}

darknet::~darknet() throw()
{
    m_io_service->stop();
    m_threads->join_all();
    
}

void
darknet::start_resolving(boost::shared_ptr<ResolverQuery> rq)
{
    using namespace json_spirit;
    Object jq = rq->get_json();
    ostringstream querystr;
    write_formatted( jq, querystr );
    msg_ptr msg(new LameMsg(querystr.str(), SEARCHQUERY));
    start_search(msg);
}

/// ---

bool 
darknet::new_incoming_connection( connection_ptr conn )
{
    // Send welcome message, containing our identity
    msg_ptr lm(new LameMsg(conf()->name(), WELCOME));
    send_msg(conn, lm);
    return true;
}

bool 
darknet::new_outgoing_connection( connection_ptr conn, boost::asio::ip::tcp::endpoint &endpoint )
{
    cout << "New connection to remote servent setup." << endl;
    return true;
}

void
darknet::send_identify(connection_ptr conn )
{
    msg_ptr lm(new LameMsg(conf()->name(), IDENTIFY));
    send_msg(conn, lm);
}

void 
darknet::write_completed(connection_ptr conn, msg_ptr msg)
{
    // Nothing to do really.
    //std::cout << "write_completed("<< msg->toString() <<")" << endl;
}

void
darknet::connection_terminated(connection_ptr conn)
{
	cout << "Connection terminated: " << conn->username() << endl;
	unregister_connection(conn->username());
	conn->set_authed(false);
    conn->close();
}

/// Handle completion of a read operation.
/// Typically a new message just arrived.
/// @return true if connection should remain open
bool 
darknet::handle_read(   const boost::system::error_code& e, 
                    msg_ptr msg, 
                    connection_ptr conn)
{
    if(e)
    {
	    connection_terminated(conn);
	    return false;
    }
    
    ///cout << "handle_read( msgtype="<< msg->msgtype() << " payload: "<< msg->toString() <<")" << endl;
    /// Auth stuff first:
    if(msg->msgtype() == WELCOME)
    { // an invitation to identify ourselves
        cout << "rcvd welcome message from '"<<msg->payload()<<"'" << endl;
        register_connection(msg->payload(), conn);
        send_identify(conn);
        return true;
    }
    
    if(msg->msgtype() == IDENTIFY)
    {
        string username = msg->payload();
        register_connection(username, conn);
        return true;
    }
    else if(!conn->authed()) //msg->msgtype()!=WELCOME)
    {
        /// if not authed, kick em
        if(!conn->authed())
        {
            cout << "Kicking, didn't auth" << endl;
            if(conn->username().length())
            {
                unregister_connection(conn->username());
            }
            conn->set_authed(false);
            conn->close();
            return false;
        }
    }
    
    //cout << "RCVD('"<<conn->username()<<"')\t" << msg->toString()<<endl;
    /// NORMAL STATE MACHINE OPS HERE:
    switch(msg->msgtype())
    {
        case SEARCHQUERY:
            return handle_searchquery(conn,msg);
        case SEARCHRESULT:
            return handle_searchresult(conn, msg);
        case SIDREQUEST:
            m_io_service->post( boost::bind( 
                &darknet::handle_sidrequest, this,
                conn, msg ));
            return true;
        case SIDDATA:
            //m_io_service->post( boost::bind( 
            //    &darknet::handle_siddata, this, conn, msg));
            handle_siddata(conn,msg);
            return true;
        default:
            cout << "UNKNOWN MSG! " << msg->toString() << endl;
            return true;
    }
}

bool
darknet::handle_searchquery(connection_ptr conn, msg_ptr msg)
{
    using namespace json_spirit;
    boost::shared_ptr<ResolverQuery> rq;
    try
    {
        Value mv;
        if(!read(msg->payload(), mv)) 
        {
            cout << "Darknet: invalid JSON in this message, discarding." << endl;
            return false; // invalid json = disconnect them.
        }
        Object qo = mv.get_obj();
        rq = ResolverQuery::from_json(qo);
    } 
    catch (...) 
    {
        cout << "Darknet: invalid search json, discarding" << endl;
        return true; //TODO maybe false - cut off the connection?
    }
    
    if(resolver()->query_exists(rq->id()))
    {
        //cout << "Darknet: discarding search message, QID already exists: " << rq->id() << endl;
        return true;
    }

    // register source for this query, so we know where to 
    // send any replies to.
    set_query_origin(rq->id(), conn);

    // dispatch search with our callback handler:
    rq_callback_t cb = boost::bind(&darknet::send_response, this, _1, _2);
    query_uid qid = resolver()->dispatch(rq, cb);
    
    assert(rq->id() == qid);
    
    /*
        schedule search to be fwded to our peers - this will abort if
        the query has been solved before it fires anyway.
          
        The 100ms delay is intentional - it means cancellation messages
        can reach the search frontier immediately (fwded with no delay)
    */
    boost::shared_ptr<boost::asio::deadline_timer> 
        t(new boost::asio::deadline_timer( m_work->get_io_service() ));
    t->expires_from_now(boost::posix_time::milliseconds(100));
    // pass the timer pointer to the handler so it doesnt autodestruct:
    t->async_wait(boost::bind(&darknet::fwd_search, this,
                                boost::asio::placeholders::error, 
                                conn, msg, t, qid));

    return true;
}

void
darknet::fwd_search(const boost::system::error_code& e,
                     connection_ptr conn, msg_ptr msg,
                     boost::shared_ptr<boost::asio::deadline_timer> t,
                     query_uid qid)
{
    if(e)
    {
        cout << "Error from timer, not fwding: "<< e.value() << " = " << e.message() << endl;
        return;
    }
    // bail if already solved (probably from our locallibrary resolver)
    if(resolver()->rq(qid)->solved())
    {
        //cout << "Darknet: not relaying solved search: " << qid << endl;
        return;
    }
    
    // TODO check search is still active
    cout << "Forwarding search.." << endl;
    typedef std::pair<string,connection_ptr> pair_t;
    BOOST_FOREACH(pair_t item, m_connections)
    {
        if(item.second == conn)
        {
            cout << "Skipping " << item.first << " (origin)" << endl;
            continue;
        }
        cout << "\tFwding to: " << item.first << endl;
        send_msg(item.second, msg);
    }
}

// fired when a new result is available for a running query:
void
darknet::send_response( query_uid qid, 
                        boost::shared_ptr<ResolvedItem> rip)
{
    connection_ptr origin_conn = get_query_origin(qid);
    // relay result if the originating connection still active:
    if(origin_conn)
    {
        cout << "Relaying search result to " 
             << origin_conn->username() << endl;
        Object response;
        response.push_back( Pair("qid", qid) );
        response.push_back( Pair("result", rip->get_json()) );
        ostringstream ss;
        write_formatted( response, ss );
        msg_ptr resp(new LameMsg(ss.str(), SEARCHRESULT));
        send_msg(origin_conn, resp);
    }
}

bool
darknet::handle_searchresult(connection_ptr conn, msg_ptr msg)
{
    //cout << "Got search result: " << msg->toString() << endl;
    using namespace json_spirit;
    // try and parse it as json:
    Value v;
    if(!read(msg->payload(), v)) 
    {
        cout << "Darknet: invalid JSON in this message, discarding." << endl;
        return false; // invalid json = disconnect.
    }
    Object o = v.get_obj();
    map<string,Value> r;
    obj_to_map(o,r);
    if(r.find("qid")==r.end() || r.find("result")==r.end())
    {
        cout << "Darknet, malformed search response, discarding." << endl;
        return false; // malformed = disconnect.
    }
    query_uid qid = r["qid"].get_str();
    Object resobj = r["result"].get_obj();
    boost::shared_ptr<PlayableItem> pip;
    try
    {
        pip = PlayableItem::from_json(resobj);
        
    }
    catch (...)
    {
        cout << "Darknet: Missing fields in response json, discarding" << endl;
        return true; // could just be incompatible version, not too bad. don't disconnect.
    }
    boost::shared_ptr<StreamingStrategy> s(
                            new DarknetStreamingStrategy( this, conn, pip->id() ));
    pip->set_streaming_strategy(s);
    vector< boost::shared_ptr<ResolvedItem> > vr;
    vr.push_back(pip);
    report_results(qid, vr, name());
    // we've already setup a callback, which will be fired when we call report_results.    
    return true;
}

// asks remote host to start streaming us data for this sid
void
darknet::start_sidrequest(connection_ptr conn, source_uid sid, 
                             boost::function<bool (msg_ptr)> handler)
{
    m_sidhandlers[sid] = handler;
    msg_ptr msg(new LameMsg(sid, SIDREQUEST));
    send_msg(conn, msg);
}

// a peer has asked us to start streaming something:
bool
darknet::handle_sidrequest(connection_ptr conn, msg_ptr msg)
{
    source_uid sid = msg->payload();
    cout << "Darknet request for sid: " << sid << endl;
    boost::shared_ptr<ResolvedItem> rip = resolver()->get_ri(sid);
    
    pi_ptr pip = boost::dynamic_pointer_cast<PlayableItem>(rip);
    if( !pip )
        return false;
    
    // We send SIDDATA msgs, where the payload is a sid_header followed
    // by the audio data.
    char buf[8194]; // this is the lamemsg payload.
    int len, total=0;
    sid_header sheader;
    memcpy((char*)&sheader.sid, sid.c_str(), 36);
    // put sheader at the start of our buffer:
    memcpy((char*)&buf, (char*)&sheader, sizeof(sid_header));
    
    if(pip) // send data:
    {
        cout << "-> PlayableItem: " << pip->artist() 
             << " - " << pip->track() << endl;
        boost::shared_ptr<StreamingStrategy> ss = pip->streaming_strategy();
        cout << "-> " << ss->debug() << endl;
        cout << "-> source: '"<< pip->source() <<"'" << endl;
        cout << "Sending siddata packets: header.sid:'" 
            << sid << "'" << endl;
        // this will be the offset where we write audio data,
        // to leave the sid_header intact at the start:
        char * const buf_datapos = ((char*)&buf) + sizeof(sid_header);
        // read audio data into buffer at the data offset:
        while ((len = ss->read_bytes( buf_datapos,
                                      sizeof(buf)-sizeof(sid_header))
               )>0)
        {
            total+=len;
            string payload((const char*)&buf, sizeof(sid_header)+len);
            msg_ptr msgp(new LameMsg(payload, SIDDATA));
            send_msg(conn, msgp);
        }
    }
    else
    {
        cout << "No playableitem for sid '"<<sid<<"'" << endl;
        // send empty packet anyway, to signify EOS
        // TODO possibly send an msgtype=error msg
    }
    
    // send empty siddata to signify end of stream
    cout << "Sending end part. Transferred " << total << " bytes" << endl;
    string eostream((char*)&buf, sizeof(sid_header));
    msg_ptr msge(new LameMsg(eostream, SIDDATA));
    send_msg(conn, msge);
    cout << "Darknet: done streaming sid" << endl; 
    return true;
}

bool
darknet::handle_siddata(connection_ptr conn, msg_ptr msg)
{
    // first part of payload is sid_header:
    sid_header sheader;
    memcpy(&sheader, msg->payload().c_str(), sizeof(sid_header));
    source_uid sid = string((char *)&sheader.sid, 36);
    //cout << "Rcvd part for " << sid << endl; 
    if(m_sidhandlers.find(sid) == m_sidhandlers.end())
    {
        cout << "Invalid sid("<<sid<<"), discarding" << endl;
        // TODO send cancel message
        return true;
    } 
    // pass msg to appropriate handler
    m_sidhandlers[sid](msg);    
    return true;
}


/// sends search query to all connections.
void
darknet::start_search(msg_ptr msg)
{
    //cout << "Searching... " << msg->toString() << endl;
    typedef std::pair<string,connection_ptr> pair_t;
    BOOST_FOREACH(pair_t item, m_connections)
    {
        cout << "\tSending to: " << item.first << endl;
        send_msg(item.second, msg);
    }
}

/// associate username->conn
void
darknet::register_connection(string username, connection_ptr conn)
{
    cout << "Registered connection for: " << username << endl;
    m_connections[username]=conn;
    conn->set_authed(true);
    conn->set_username(username);
}

void 
darknet::unregister_connection(string username)
{
    m_connections.erase(username);
}

void 
darknet::send_msg(connection_ptr conn, msg_ptr msg)
{
    // msg is passed thru to the callback, so it stays referenced and isn't GCed until sent.
    conn->async_write(msg);/*,
                      boost::bind(&Servent::handle_write, m_servent,
                      boost::asio::placeholders::error, conn, msg));*/
}

// web interface:
string 
darknet::http_handler(const playdar_request& req,
                      playdar::auth * pauth)
{
    cout << "http_handler called on darknet. pauth = " << pauth << endl;
    if( req.postvar_exists("formtoken") &&
        req.postvar_exists("newaddr") &&
        req.postvar_exists("newport") &&
        pauth->consume_formtoken(req.postvar("formtoken")) )
    {
        string addr = req.postvar("newaddr");
        unsigned short port = boost::lexical_cast<unsigned short>(req.postvar("newport"));
        boost::asio::ip::address_v4 ip = boost::asio::ip::address_v4::from_string(addr);
        boost::asio::ip::tcp::endpoint ep(ip, port);
        servent()->connect_to_remote(ep);
    }
    
    typedef pair<string, connection_ptr_weak> pair_t;
    ostringstream os;
    os  << "<h2>Darknet Settings</h2>" << endl
        << "<form method=\"post\" action=\"\">" << endl
        << "Connect "
        << "IP: <input type=\"text\" name=\"newaddr\" />"
        << "Port: <input type=\"text\" name=\"newport\" value=\"9999\"/>"
        << "<input type=\"hidden\" name=\"formtoken\" value=\""
            << pauth->gen_formtoken() << "\"/>"
        << " <input type=\"submit\" value=\"Connect to remote servent\" />"
        << "</form>" << endl
        ;
    os  << "<h3>Current Connections</h3>"    
        << "<table>"
        << "<tr style=\"font-weight:bold;\">"
        << "<td>Username</td><td>Msg Queue Size</td><td>Address</td></tr>";
        
    BOOST_FOREACH(pair_t p, connections())
    {
        connection_ptr conn(p.second);
        boost::asio::ip::tcp::endpoint remote_ep = conn->socket().remote_endpoint();
        os  << "<tr>"
            << "<td>" << p.first << "</td>"
            << "<td>" << conn->writeq_size() << "</td>"
            << "<td>" << remote_ep.address().to_string() 
            <<           ":" << remote_ep.port() << "</td>"
            << "</tr>";
    }
    os  << "</table>" 
        << endl; 
    //cout << os.str();
    return os.str();
}


EXPORT_DYNAMIC_CLASS( darknet )
