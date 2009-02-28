#include "msgs.h"
#include "protocol.h"
#include "servent.h"

#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;

namespace playdar {
namespace darknet {



Protocol::Protocol(string u, boost::asio::io_service & io)
    : m_work(io)
{
    m_username = u;
    boost::thread thr(boost::bind(&boost::asio::io_service::run, &io));
    thr.detach();
}

Protocol::~Protocol()
{
   //m_io_ptr->stop();
}

void 
Protocol::init(Servent *sv)
{
    m_servent = sv;
}

bool 
Protocol::new_incoming_connection( connection_ptr conn )
{
    // Send welcome message, containing our identity
    msg_ptr lm(new LameMsg(m_username, WELCOME));
    send_msg(conn, lm);
    return true;
}

bool 
Protocol::new_outgoing_connection( connection_ptr conn, boost::asio::ip::tcp::endpoint &endpoint )
{
    cout << "New connection to remote servent setup." << endl;
}

void
Protocol::send_identify(connection_ptr conn )
{
    msg_ptr lm(new LameMsg(m_username, IDENTIFY));
    send_msg(conn, lm);
}

void 
Protocol::write_completed(connection_ptr conn, msg_ptr msg)
{
    // Nothing to do really.
    std::cout << "write_completed("<< msg->toString() <<")" << endl;
}

void
Protocol::connection_terminated(connection_ptr conn)
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
Protocol::handle_read(   const boost::system::error_code& e, 
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
Protocol::handle_searchquery(connection_ptr conn, msg_ptr msg)
{
    if(
        !(m_username=="userA" && msg->payload()=="searchA") &&
        !(m_username=="userB" && msg->payload()=="searchB") &&
        !(m_username=="userC" && msg->payload()=="searchC") 
       )
    {
        cout << "No matches, will fwd it after delay.." << endl;
        boost::shared_ptr<boost::asio::deadline_timer> 
            t(new boost::asio::deadline_timer( m_work.get_io_service() ));
        t->expires_from_now(boost::posix_time::seconds(5));
        // pass the timer pointer to the handler so it doesnt autodestruct:
        t->async_wait(boost::bind(&Protocol::fwd_search, this,
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
Protocol::fwd_search(const boost::system::error_code& e,
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
Protocol::handle_searchresult(connection_ptr conn, msg_ptr msg)
{
    cout << "Got search result: " << msg->toString() << endl;
    return true;
}

/// sends search query to all connections.
void
Protocol::start_search(msg_ptr msg)
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
Protocol::register_connection(string username, connection_ptr conn)
{
    cout << "Registered connection for: " << username << endl;
    m_connections[username]=conn;
    conn->set_authed(true);
    conn->set_username(username);
}

void 
Protocol::unregister_connection(string username)
{
    m_connections.erase(username);
}

void 
Protocol::send_msg(connection_ptr conn, msg_ptr msg)
{
    conn->async_write(msg,
                      boost::bind(&Servent::handle_write, m_servent,
                      boost::asio::placeholders::error, conn, msg));
}


}} // namespaces
