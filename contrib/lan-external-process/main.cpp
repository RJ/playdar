#include "playdar/playdar_resolver_process.hpp"

// funcs
bool process_msg( const string & msg );
void run();
void async_send(boost::asio::ip::udp::endpoint * remote_endpoint, const string& message);
void handle_send(const boost::system::error_code& error, size_t bytes_recvd, char * scratch );
void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd);
void report_results( query_uid qid, boost::shared_ptr<PlayableItem> pip );
void send_response( query_uid qid, pi_ptr pip, boost::asio::ip::udp::endpoint sep );
void broadcast_search( rq_ptr rq, rq_callback_t cb );

// global vars
char data_[MAXBUF];
boost::asio::ip::udp::socket * socket_;
boost::asio::ip::udp::endpoint sender_endpoint_;
boost::asio::ip::udp::endpoint * broadcast_endpoint_;
boost::asio::io_service * io_service;
boost::thread * responder_thread;

map< query_uid, rq_callback_t > qid_handlers; // handler functions for PIs returned by playdar by QID

//
int main(int argc, char **argv)
{
    cerr    << "External UDP LAN Resolver says hello." << endl;

    playdar_send_settings( "UDP Ext Resolver", 99, 50 );

    // init UDP multicast stuff:
    broadcast_endpoint_ = 
        new boost::asio::ip::udp::endpoint
         (  boost::asio::ip::address::from_string
            (/*conf()->get<string> ("plugins.lan_udp.multicast")*/"239.255.0.1"), 
           /*conf()->get<int>("plugins.lan_udp.port")*/ 8888);
    responder_thread = new boost::thread(&run);


    // enter main loop for handling stdin:
    char buffer[MAXBUF];
    boost::uint32_t len;
    while( !cin.fail() && !cin.eof() )
    {
        // get length header:
        cin.read( (char*)&len, 4 );
        if(cin.fail() || cin.eof())
        {
            cerr << "Stdin stream failed" << endl;
            return 1;
        }
        len = ntohl(len);
        if(len > MAXBUF)
        {
            cerr << "Payload length header too big! (" << len << ")" << endl;
            return 2;
        }
        // get msg payload:
        cin.read( (char*)&buffer, len );
        if(cin.fail() || cin.eof())
        {
            cerr << "Stdin stream failed" << endl;
            return 1;
        }
        string msg( (char*)&buffer, len );
        if(!process_msg( msg )) break;
    }

    cerr << "Bailed from main loop due to error. Exiting." << endl;
    // cleanup:
    io_service->stop();
    responder_thread->join();
    delete(socket_);
    delete(broadcast_endpoint_);
    return 99;
}

bool process_msg( const string& msg  )
{
    cerr << "Process msg:" << msg << endl << endl;
    Value j;
    if(!read(msg, j) || j.type() != obj_type)
    {
        cerr << "Aborting, invalid JSON." << endl;
        return false;
    }
    Object o = j.get_obj();
    map<string, Value> m;
    obj_to_map(o, m);
    if(m.find("artist")!=m.end()) // it's a query TODO this sucks
    {
        rq_ptr rq;
        try
        {
            rq = ResolverQuery::from_json(o);
            // dispatch query. will be dropped if qid already exists
            rq_callback_t cb = boost::bind(&report_results, _1, _2);
            broadcast_search( rq, cb );
            return true;
        }
        catch(...)
        {
            cerr << "Invalid query input from playdar" << endl;
            return false;
        }
    }
    
    cerr << "Unhandled msg rcvd from playdar." << endl;
    return true;

}

// tell playdar to start searching for something
void broadcast_search( rq_ptr rq, rq_callback_t cb )
{
    if(!playdar_should_dispatch( rq->id() )) return;
    // register callback if we get results for this qid:
    if(cb) qid_handlers[rq->id()] = cb;
    cerr << "Dispatching (" << rq->from_name() << "):\t" << rq->str() << endl;
    // send UDP search message:
    Object jq;
    jq.push_back( Pair("from_name", "ext udp script TODO"/*conf()->name()*/) );
    jq.push_back( Pair("query", rq->get_json()) );
    ostringstream querystr;
    write_formatted( jq, querystr );
    async_send(broadcast_endpoint_, querystr.str());
}

// run thread for UDP stuff
// sets up socket, joins multicast UDP group, listens for UDP msg.
// called in a thread, it calls io_service->run() so never returns.
void run()
{
    io_service = new boost::asio::io_service;
    socket_ = new boost::asio::ip::udp::socket(*io_service);
    // Create the socket so that multiple may be bound to the same address.
    boost::asio::ip::udp::endpoint listen_endpoint( boost::asio::ip::address::from_string("0.0.0.0"), 8888);
    socket_->open(listen_endpoint.protocol());
    socket_->set_option(boost::asio::ip::udp::socket::reuse_address(true));
    socket_->bind(listen_endpoint);
    // Join the multicast group.
    socket_->set_option( boost::asio::ip::multicast::join_group(
                            boost::asio::ip::address::from_string("239.255.0.1")));
    socket_->async_receive_from(
            boost::asio::buffer(data_, MAXBUF), sender_endpoint_,
            boost::bind(&handle_receive_from,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    // now we're listening.
    cerr << "DL UDP Resolver is online udp://" 
         << socket_->local_endpoint().address() << ":"
         << socket_->local_endpoint().port()
         << endl;
    // announce our presence to the LAN:
    string hello = "OHAI ";
    hello += "TODOname"; //conf()->name();
    async_send(broadcast_endpoint_, hello); 
    io_service->run(); 
    // won't get here, io_service will run forever.
}


// called when we rcv a UDP msg
void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
{
    if (!error)
    {
        std::ostringstream msgs;
        msgs.write(data_, bytes_recvd);
        std::string msg = msgs.str();

        boost::asio::ip::address sender_address = sender_endpoint_.address();

        do
        {
            //cerr << "LAN_UDP: Received multicast message (from " 
            //     << sender_address.to_string() << ":" << sender_endpoint_.port() << "):"  
            //     << msg << endl;
    
            // join leave msg, for debugging on lan
            if(msg.substr(0,5)=="OHAI " || msg.substr(0,8)=="KTHXBYE ")
            {
                cerr << "INFO Presence msg: " << msg << endl;
                break;
            }
     
            // try and parse it as json:
            Value mv;
            if(!read(msg, mv)) 
            {
                cerr << "LAN_UDP: invalid JSON in this message, discarding." << endl;
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
                    cerr << "LAN_UDP: missing fields in JSON query object, discarding" << endl;
                    break; 
                }

                // dispatch query to playdar if it's not already running:
                if(playdar_should_dispatch(rq->id()))
                {
                    qid_handlers[rq->id()] = boost::bind(&send_response, _1, _2, sender_endpoint_);
                    playdar_dispatch( rq );
                }
            }
            else if(r.find("qid")!=r.end()) // RESPONSE 
            {
                Object resobj = r["result"].get_obj();
                map<string,Value> resobj_map;
                obj_to_map(resobj, resobj_map);
                query_uid qid = r["qid"].get_str();
                // if we could ask "query_exists?" we could abort if it doesnt at this point.
                //cerr << "LAN_UDP: Got udp response." <<endl;
                boost::shared_ptr<PlayableItem> pip;
                try
                {
                    pip = PlayableItem::from_json(resobj);
                }
                catch (...)
                {
                    cerr << "LAN_UDP: Missing fields in response json, discarding" << endl;
                    break;
                }
                ostringstream rbs;
                rbs << "http://"
                    << sender_endpoint_.address()
                    << ":"
                    << sender_endpoint_.port();
                string url = rbs.str();
                url += "/sid/";
                url += pip->id();
                pip->set_url( url );
                // anything on udp multicast must be pretty fast:
                pip->set_preference((float)0.9); 
                vector< boost::shared_ptr<PlayableItem> > v;
                v.push_back(pip);
                playdar_report_results(qid, v);
                cerr    << "INFO Result from '" << pip->source()
                        <<"' for '"<< pip->artist() <<"' - '"
                        << pip->track() << "' [score: "<< pip->score() <<"]" 
                        << endl;
            }
            
        }while(false);
                
        socket_->async_receive_from(
                boost::asio::buffer(data_, MAXBUF), sender_endpoint_,
                boost::bind(&handle_receive_from,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        cerr << "Some error for udp" << endl;
    }
}

void
send_response( query_uid qid, 
               boost::shared_ptr<PlayableItem> pip,
               boost::asio::ip::udp::endpoint sep )
{
    cerr << "LAN_UDP responding for " << qid << " to: " 
         << sep.address().to_string() 
         << " score: " << pip->score()
         << endl;
    Object response;
    response.push_back( Pair("qid", qid) );
    Object result = pip->get_json();
    response.push_back( Pair("result", result) );
    ostringstream ss;
    write_formatted( response, ss );
    async_send(&sep, ss.str());
}


void
report_results( query_uid qid, 
                boost::shared_ptr<PlayableItem> pip )
{
    cerr << "Report result for " << qid << " : " 
         << " score: " << pip->score()
         << endl;
    Object o;
    o.push_back( Pair("_msgtype", "results") );
    o.push_back( Pair("qid", qid) );
    Array arr;
    arr.push_back( pip->get_json() );
    o.push_back( Pair("results", arr) );

    playdar_write( o );
}


void 
async_send(boost::asio::ip::udp::endpoint * remote_endpoint, const string& message)
{
    if(message.length()>1200)
    {
        cerr << "WARNING outgoing UDP message is rather large, haven't tested this, discarding." << endl;
        return;
    }
    //cerr << "UDPsend[" << remote_endpoint->address().to_string() 
    //     << ":" << remote_endpoint->port() << "]"
    //     << "(" << message << ")" << endl;
    char * buf = (char*)malloc(message.length());
    memcpy(buf, message.data(), message.length());
    socket_->async_send_to(     
            boost::asio::buffer(buf,message.length()), 
            *remote_endpoint,
            boost::bind(&handle_send,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred,
                        buf));
}

void handle_send(const boost::system::error_code& error, size_t bytes_recvd, char * scratch )
{
    // free the memory that was holding the message we just sent.
    if(scratch)
    {
        free(scratch);
    }
}

