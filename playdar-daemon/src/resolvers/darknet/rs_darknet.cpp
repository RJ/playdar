#include <iostream>                        
#include <boost/asio.hpp>                  
#include <boost/lexical_cast.hpp>          
#include <boost/thread.hpp>                
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp> 
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "resolvers/darknet/rs_darknet.h"

#include "resolvers/darknet/msgs.h"
#include "resolvers/darknet/servent.h"

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
    //m_io_service_p  = boost::shared_ptr<boost::asio::io_service>(new boost::asio::io_service);
    m_servent = boost::shared_ptr< Servent >(new Servent(*m_io_service, port, this));
    // start io_services:
    boost::thread thr(boost::bind(&RS_darknet::start_io, this, m_io_service));
    //boost::thread thrp(boost::bind(&RS_darknet::start_io, this, m_io_service_p));
    cout << "Darknet resolver coming online.."<<endl;
    // get peers:
    string remote_ip = app()->popt()["resolver.darknet.remote_ip"].as<string>();
    unsigned short remote_port = app()->popt()["resolver.darknet.remote_port"].as<int>();
    cout << "Attempting peer connect: " << remote_ip << ":" << remote_port << endl;
    boost::asio::ip::address_v4 ipaddr = boost::asio::ip::address_v4::from_string(remote_ip);
    boost::asio::ip::tcp::endpoint ep(ipaddr, remote_port);
    m_servent->connect_to_remote(ep);
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
    std::cout << "write_completed("<< msg->toString() <<")" << endl;
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
    

    if(msg->msgtype() == CONNECTTO)
    {
        boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 1235);
        m_servent->connect_to_remote(ep);
        return true;
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
        string username = msg->toString();
        register_connection(username, conn);
        return true;
    }
    else if(msg->msgtype()!=WELCOME)
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
    
    cout << "RCVD('"<<conn->username()<<"')\t" << msg->toString()<<endl;
    /// NORMAL STATE MACHINE OPS HERE:
    switch(msg->msgtype())
    {
        case SEARCHQUERY:
            return handle_searchquery(conn,msg);
        case SEARCHRESULT:
            return handle_searchresult(conn, msg);
        default:
            cout << "UNKNOWN MSG! " << msg->toString() << endl;
            return true;
    }
}

bool
RS_darknet::handle_searchquery(connection_ptr conn, msg_ptr msg)
{
    if(
        !(app()->name()=="userA" && msg->payload()=="searchA") &&
        !(app()->name()=="userB" && msg->payload()=="searchB") &&
        !(app()->name()=="userC" && msg->payload()=="searchC") 
       )
    {
        cout << "No matches, will fwd it after delay.." << endl;
        boost::shared_ptr<boost::asio::deadline_timer> 
            t(new boost::asio::deadline_timer( m_work->get_io_service() ));
        t->expires_from_now(boost::posix_time::seconds(5));
        // pass the timer pointer to the handler so it doesnt autodestruct:
        t->async_wait(boost::bind(&RS_darknet::fwd_search, this,
                                 boost::asio::placeholders::error, 
                                 conn, msg, t));
        return true;
    }
    // search result!
    //cout << "searchquery: " << msg->toString() << endl;
    string r = "result for: ";
    r+=msg->toString();
    msg_ptr resp(new LameMsg(r, SEARCHRESULT));
    send_msg(conn, resp);
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
    cout << "Got search result: " << msg->toString() << endl;
    return true;
}

/// sends search query to all connections.
void
RS_darknet::start_search(msg_ptr msg)
{
    cout << "Searching... " << msg->toString() << endl;
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
    conn->async_write(msg,
                      boost::bind(&Servent::handle_write, m_servent,
                      boost::asio::placeholders::error, conn, msg));
}


        