#include "application/application.h"
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include "json_spirit/json_spirit.h"

#include "resolvers/rs_lan_udp.h"
#include "library/library.h"
#include "udp_sender.h"

RS_lan_udp::RS_lan_udp(MyApplication * a)
    :ResolverService(a)
{
    string hello = "OHAI ";
    hello += app()->name();
    boost::thread thr( &UDPSender::send,
                    app()->multicast_ip(), 
                    app()->multicast_port(), 
                    hello );
    boost::thread m_responder_thread(&RS_lan_udp::init, this);
}

RS_lan_udp::~RS_lan_udp()
{
    // currently this won't fire if you just control+C
    // need to trap exit signals etc
    cout << "lan_udp resolver shutting down" << endl;
    string hello = "KTHXBYE ";
    hello += app()->name();
    UDPSender::send(app()->multicast_ip(), 
                    app()->multicast_port(), 
                    hello );
    delete(socket_);
}

void
RS_lan_udp::start_resolving(boost::shared_ptr<ResolverQuery> rq)
{
    using namespace json_spirit;
    Object jq;
    jq.push_back( Pair("reply_ip", app()->private_ip().to_string()) );
    jq.push_back( Pair("reply_port", app()->multicast_port()) );
    jq.push_back( Pair("from_name", app()->name()) );
    jq.push_back( Pair("query", rq->get_json()) );
    
    ostringstream querystr;
    write_formatted( jq, querystr );
    boost::thread thr( &UDPSender::send,
                       app()->multicast_ip(), 
                       app()->multicast_port(), 
                       querystr.str() );
}

void 
RS_lan_udp::init()
{
    cout << "UDP resolver started on udp://0.0.0.0:" << app()->multicast_port() << endl; 
    boost::asio::io_service io_service;
    start_listening(io_service, boost::asio::ip::address::from_string("0.0.0.0"), 
                    app()->multicast_ip(), app()->multicast_port());

    io_service.run();

}        
            
void 
RS_lan_udp::start_listening(boost::asio::io_service& io_service,
        const boost::asio::ip::address& listen_address,
        const boost::asio::ip::address& multicast_address,
        const short multicast_port)
{ 
    socket_ = new boost::asio::ip::udp::socket(io_service);
    // Create the socket so that multiple may be bound to the same address.
    boost::asio::ip::udp::endpoint listen_endpoint(
            listen_address, multicast_port);
    socket_->open(listen_endpoint.protocol());
    socket_->set_option(boost::asio::ip::udp::socket::reuse_address(true));
    socket_->bind(listen_endpoint);

    // Join the multicast group.
    socket_->set_option(
            boost::asio::ip::multicast::join_group(multicast_address));

    socket_->async_receive_from(
            boost::asio::buffer(data_, max_length), sender_endpoint_,
            boost::bind(&RS_lan_udp::handle_receive_from, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
                
}


void 
RS_lan_udp::handle_receive_from(const boost::system::error_code& error,
        size_t bytes_recvd)
{
    if (!error)
    {
        std::ostringstream msgs;
        msgs.write(data_, bytes_recvd);
        std::string msg = msgs.str();

        boost::asio::ip::address sender_address = sender_endpoint_.address();
        unsigned short sender_port = sender_endpoint_.port();

        // TODO: reusing the JSON parser would be better than this crap:

        do
        {
            
            if( sender_address.to_string() == "127.0.0.1" ||
                sender_address == app()->private_ip() ||
                sender_address == app()->public_ip() )
            {
                //cout << "* Ignoring udp msg from self" << endl;
                break;
            }
            
            cout    << "LAN_UDP: Received multicast message (from " 
                    << sender_address.to_string() << ":" << sender_port << "):" << endl; 
                    //<< endl << msg << endl << endl;
    
            // special (hacked in) join leave msg, for fun debugging on lan
            if(msg.substr(0,5)=="OHAI " || msg.substr(0,8)=="KTHXBYE ")
            {
                cout << "INFO Online/offline msg: " << msg << endl;
                break;
            }
     
            using namespace json_spirit;
            // try and parse it as json:
            Value mv;
            if(!read(msg, mv)) 
            {
                cout << "LAN_UDP: invalid JSON in this message, discarding." << endl;
                break; // Invalid JSON, ignore it.
            }
            Object qo = mv.get_obj();
            map<string,Value> r;
            obj_to_map(qo,r);
            
            if(r.find("query")!=r.end()) // REQUEST / NEW QUERY
            {
                Object qryobj = r["query"].get_obj();
                boost::shared_ptr<ResolverQuery> rq;
                try
                {
                    rq = ResolverQuery::from_json(qryobj);
                } 
                catch (...) 
                {
                    cout << "LAN_UDP: missing fields in JSON query object, discarding" << endl;
                    break; 
                }
                
                if(app()->resolver()->query_exists(rq->id()))
                {
                    cout << "LAN_UDP: discarding message, QID already exists: " << rq->id() << endl;
                    break;
                }
                
                string replyip = r["reply_ip"].get_str();
                int replyport = r["reply_port"].get_int();
                
                boost::asio::ip::address_v4 response_addr = 
                    boost::asio::ip::address_v4::from_string(replyip);
                unsigned short response_port  = (unsigned short)(replyport);

                query_uid qid = app()->resolver()->dispatch(rq, true);
                vector< boost::shared_ptr<PlayableItem> > pis = app()->resolver()->get_results(qid);
                // TODO end/delete query
                // Format results as JSON and respond:
                if(pis.size()>0){
                    BOOST_FOREACH(boost::shared_ptr<PlayableItem> & pip, pis)
                    {
                        string url = app()->httpbase();
                        url += "/sid/" + pip->id();
                        Object response;
                        response.push_back( Pair("qid", qid) );
                        Object result = pip->get_json();                        
                        result.push_back( Pair("url", url) ); // poke in the URL for now.
                        response.push_back( Pair("result", result) );
                        ostringstream ss;
                        write_formatted( response, ss );
                        cout << "LAN_UDP: Sending response, hit of score " << pip->score() << endl;
                        UDPSender::send(response_addr, response_port, ss.str());
                    }
                }else{
                    cout << "LAN_UDP: Not responding, nothing matched." << endl;
                }   
            }
            else if(r.find("qid")!=r.end()) // RESPONSE 
            {
                Object resobj = r["result"].get_obj();
                map<string,Value> resobj_map;
                obj_to_map(resobj, resobj_map);
                query_uid qid = r["qid"].get_str();
                if(!app()->resolver()->query_exists(qid))
                {
                    cout << "LAN_UDP: Ignoring response - QID invalid or expired" << endl;
                    break;
                }
                cout << "LAN_UDP: Got udp response." <<endl;
                boost::shared_ptr<PlayableItem> pip;
                try
                {
                    pip = PlayableItem::from_json(resobj);
                }
                catch (...)
                {
                    cout << "LAN_UDP: Missing fields in response json, discarding" << endl;
                    break;
                }
                string url      = resobj_map["url"].get_str();
                boost::shared_ptr<StreamingStrategy> s(new HTTPStreamingStrategy(url));
                pip->set_streaming_strategy(s);
                // anything on udp multicast must be pretty fast:
                pip->set_preference((float)0.9); 
                vector< boost::shared_ptr<PlayableItem> > v;
                v.push_back(pip);
                report_results(qid, v);
                cout    << "INFO Result from '" << pip->source()
                        <<"' for '"<< pip->artist() <<"' - '"
                        << pip->track() << "' [score: "<< pip->score() <<"]" 
                        << endl;
            }
            
        }while(false);
                
        socket_->async_receive_from(
                boost::asio::buffer(data_, max_length), sender_endpoint_,
                boost::bind(&RS_lan_udp::handle_receive_from, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        cerr << "Some error for udp" << endl;
    }
}
