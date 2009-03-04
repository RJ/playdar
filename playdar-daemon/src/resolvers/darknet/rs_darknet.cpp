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

#include "resolvers/darknet/rs_darknet.h"

#include "resolvers/darknet/msgs.h"
#include "resolvers/darknet/servent.h"

#include "resolvers/darknet/ss_darknet.h"

/*
testing with 2 instances, one alternative collection.db

./bin/playdar -c etc/playdar.ini --app.name silverstone --app.private_ip 192.168.1.72 --resolver.lan_udp.enabled no --resolver.darknet.enabled yes --resolver.darknet.remote_ip ""

./bin/playdar -c etc/playdar.ini --app.db ./collection-small.db --app.name silverstone2 --app.private_ip 127.0.0.1 --resolver.lan_udp.enabled no --resolver.darknet.enabled yes --resolver.darknet.remote_ip 127.0.0.1 --resolver.darknet.remote_port 9999 --resolver.darknet.port 9998 --app.http_port 8899

*/
using namespace playdar::darknet;

RS_darknet::RS_darknet(MyApplication * a)
    : ResolverService(a)
{
    init();
}

void
RS_darknet::init()
{
    unsigned short port = app()->popt()["resolver.darknet.port"].as<int>();
    m_io_service    = boost::shared_ptr<boost::asio::io_service>(new boost::asio::io_service);
    m_work = boost::shared_ptr<boost::asio::io_service::work>(new boost::asio::io_service::work(*m_io_service));
    m_servent = boost::shared_ptr< Servent >(new Servent(*m_io_service, port, this));
    // start io_services:
    cout << "Darknet servent coming online on port " <<  port <<endl;
    
    boost::thread_group threads;
    for (std::size_t i = 0; i < 10; ++i)
    {
        threads.create_thread(boost::bind(
            &boost::asio::io_service::run, m_io_service.get()));
    }
    //boost::thread thr(boost::bind(&RS_darknet::start_io, this, m_io_service));
 
    // get peers:
    if(app()->popt()["resolver.darknet.remote_ip"].as<string>().length())
    {
        string remote_ip = app()->popt()["resolver.darknet.remote_ip"].as<string>();
        unsigned short remote_port = app()->popt()["resolver.darknet.remote_port"].as<int>();
        cout << "Attempting peer connect: " << remote_ip << ":" << remote_port << endl;
        boost::asio::ip::address_v4 ipaddr = boost::asio::ip::address_v4::from_string(remote_ip);
        boost::asio::ip::tcp::endpoint ep(ipaddr, remote_port);
        m_servent->connect_to_remote(ep);
    }
}

void
RS_darknet::start_resolving(boost::shared_ptr<ResolverQuery> rq)
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
RS_darknet::new_incoming_connection( connection_ptr conn )
{
    // Send welcome message, containing our identity
    msg_ptr lm(new LameMsg(app()->name(), WELCOME));
    send_msg(conn, lm);
    return true;
}

bool 
RS_darknet::new_outgoing_connection( connection_ptr conn, boost::asio::ip::tcp::endpoint &endpoint )
{
    cout << "New connection to remote servent setup." << endl;
    return true;
}

void
RS_darknet::send_identify(connection_ptr conn )
{
    msg_ptr lm(new LameMsg(app()->name(), IDENTIFY));
    send_msg(conn, lm);
}

void 
RS_darknet::write_completed(connection_ptr conn, msg_ptr msg)
{
    // Nothing to do really.
    //std::cout << "write_completed("<< msg->toString() <<")" << endl;
}

void
RS_darknet::connection_terminated(connection_ptr conn)
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
RS_darknet::handle_read(   const boost::system::error_code& e, 
                    msg_ptr msg, 
                    connection_ptr conn)
{
    if(e)
    {
	    connection_terminated(conn);
	    return false;
    }
    
    //cout << "handle_read("<< msg->toString() <<")" << endl;
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
                &RS_darknet::handle_sidrequest, this,
                conn, msg ));
            return true;
        case SIDDATA:
            //m_io_service->post( boost::bind( 
            //    &RS_darknet::handle_siddata, this, conn, msg));
            handle_siddata(conn,msg);
            return true;
        default:
            cout << "UNKNOWN MSG! " << msg->toString() << endl;
            return true;
    }
}

bool
RS_darknet::handle_searchquery(connection_ptr conn, msg_ptr msg)
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
    
    if(app()->resolver()->query_exists(rq->id()))
    {
        cout << "Darknet: discarding search message, QID already exists: " << rq->id() << endl;
        return true;
    }
    
    query_uid qid = app()->resolver()->dispatch(rq, true);
    vector< boost::shared_ptr<PlayableItem> > pis = app()->resolver()->get_results(qid);

    if(pis.size()>0)
    {
        BOOST_FOREACH(boost::shared_ptr<PlayableItem> & pip, pis)
        {
            Object response;
            response.push_back( Pair("qid", qid) );
            response.push_back( Pair("result", pip->get_json()) );
            ostringstream ss;
            write_formatted( response, ss );
            msg_ptr resp(new LameMsg(ss.str(), SEARCHRESULT));
            send_msg(conn, resp);
        }
    }
    // if we didn't just solve it, fwd to our peers, after delay.
    
    if(!rq->solved())
    {
        // register source for this query, so we know where to 
        // send any replies to.
        set_query_origin(qid, conn);
        cout << "Darknet: Query not solved, will fwd it after delay.." << endl;
        boost::shared_ptr<boost::asio::deadline_timer> 
            t(new boost::asio::deadline_timer( m_work->get_io_service() ));
        t->expires_from_now(boost::posix_time::milliseconds(100));
        // pass the timer pointer to the handler so it doesnt autodestruct:
        t->async_wait(boost::bind(&RS_darknet::fwd_search, this,
                                 boost::asio::placeholders::error, 
                                 conn, msg, t));
    }
    
    
    return true;
}

void
RS_darknet::fwd_search(const boost::system::error_code& e,
                     connection_ptr conn, msg_ptr msg,
                     boost::shared_ptr<boost::asio::deadline_timer> t)
{
    if(e)
    {
        cout << "Error from timer, not fwding: "<< e.value() << " = " << e.message() << endl;
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

bool
RS_darknet::handle_searchresult(connection_ptr conn, msg_ptr msg)
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
    //pip->set_preference((float)0.6); 
    vector< boost::shared_ptr<PlayableItem> > vr;
    vr.push_back(pip);
    report_results(qid, vr);
    /*
        Fwd search result to query origin.
        
        NB we have already done "report_results", so when the origin
        asks us for the SID, we know about it and can respond 
        accordingly. In this way, search results and streaming is
        passed back along the chain to the origin, with each node
        on the path proxing the data and hiding the previous node.
    */
    connection_ptr origin_conn = get_query_origin(qid);
    if(origin_conn)
    {
        cout << "Relaying search result to " 
             << origin_conn->username() << endl;
        send_msg(origin_conn, msg);
    }
    return true;
}

// asks remote host to start streaming us data for this sid
void
RS_darknet::start_sidrequest(connection_ptr conn, source_uid sid, 
                             boost::function<bool (msg_ptr)> handler)
{
    m_sidhandlers[sid] = handler;
    msg_ptr msg(new LameMsg(sid, SIDREQUEST));
    send_msg(conn, msg);
}

// a peer has asked us to start streaming something:
bool
RS_darknet::handle_sidrequest(connection_ptr conn, msg_ptr msg)
{
    source_uid sid = msg->payload();
    cout << "Darknet request for sid: " << sid << endl;
    boost::shared_ptr<PlayableItem> pip = app()->resolver()->get_pi(sid);
    
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
RS_darknet::handle_siddata(connection_ptr conn, msg_ptr msg)
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
RS_darknet::start_search(msg_ptr msg)
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
RS_darknet::register_connection(string username, connection_ptr conn)
{
    cout << "Registered connection for: " << username << endl;
    m_connections[username]=conn;
    conn->set_authed(true);
    conn->set_username(username);
}

void 
RS_darknet::unregister_connection(string username)
{
    m_connections.erase(username);
}

void 
RS_darknet::send_msg(connection_ptr conn, msg_ptr msg)
{
    // msg is passed thru to the callback, so it stays referenced and isn't GCed until sent.
    conn->async_write(msg);/*,
                      boost::bind(&Servent::handle_write, m_servent,
                      boost::asio::placeholders::error, conn, msg));*/
}

