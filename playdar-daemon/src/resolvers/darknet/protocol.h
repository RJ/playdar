#ifndef _PLAYDAR_DARKNET_PROTOCOL_HPP_
#define _PLAYDAR_DARKNET_PROTOCOL_HPP_

#include "msgs.h"
#include "servent.h"

using namespace std;

namespace playdar {
namespace darknet {

class Servent;

class Protocol
{
public:

    Protocol(string u, boost::asio::io_service & io);
    ~Protocol();
    
    void init(Servent *sv);
    
    /// Handle completion of a read operation.
    /// Typically a new message just arrived.
    /// @return true if connection should remain open
    bool handle_read(   const boost::system::error_code& e, 
                        msg_ptr msg, 
                        connection_ptr conn);
    void connection_terminated(connection_ptr conn);
    /// called when a msg was sent ok
    void write_completed(connection_ptr conn, msg_ptr msg);                    
    /// called when a client connects to us
    bool new_incoming_connection( connection_ptr conn );                    
    
    bool new_outgoing_connection( connection_ptr conn, boost::asio::ip::tcp::endpoint &endpoint);
    
    void send_identify(connection_ptr conn );
    void start_search(msg_ptr msg);
    void fwd_search(const boost::system::error_code& e, connection_ptr conn, msg_ptr msg, boost::shared_ptr<boost::asio::deadline_timer> t);
    
    bool handle_searchquery(connection_ptr, msg_ptr msg);
    bool handle_searchresult(connection_ptr, msg_ptr msg);
    
    void send_msg(connection_ptr conn, msg_ptr msg);
    /// associate username->conn
    void register_connection(string username, connection_ptr conn);
    void unregister_connection(string username);
    
private:
    boost::asio::io_service::work m_work;
    /// servent:
    Servent * m_servent;
    string m_username;
    /// Keep track of username -> connection
    /// this tells us how many connections are established, and to whom.
    map<string,connection_ptr> m_connections;

}; // class

}} // namespaces

#endif

