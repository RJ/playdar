#ifndef _PLAYDAR_DARKNET_CONNECTION_HPP_
#define _PLAYDAR_DARKNET_CONNECTION_HPP_

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/tuple/tuple.hpp>

#include <string>
#include <sstream>
#include <vector>

#include "msgs.h"

using namespace std;

namespace playdar {
namespace darknet {

/// This class represents a Connection to one other playdar user.
/// i.e. Use this instead of socket directly.
/// it knows how to marshal objects to and from the wire protocol
/// It keeps some state related to the Connection, eg are they authenticated.
class Connection
: public boost::enable_shared_from_this<Connection>

{
public:

    Connection(boost::asio::io_service& io_service)
        : m_socket(io_service), m_authed(false)
    {
    }
    
    ~Connection()
    {
        cout << "dtor Connection shutting down" << endl;
    }
    
    void close()
    {
        if(socket().is_open()) socket().close();
    }
    
    /// Get the underlying socket. Used for making a Connection or for accepting
    /// an incoming Connection.
    boost::asio::ip::tcp::socket& socket()
    {
        return m_socket;
    }
    
    /// Asynchronously write a data structure to the socket.
    /// should call ->serialize() on payload and write it.
    /// Must have made a copy of any data from payload before returning
    template <typename FuncTemplate>
    void async_write(msg_ptr payload, FuncTemplate handler)
    {
        if(payload->serialize(&m_outbound_data))
        {
            std::vector<boost::asio::const_buffer> buffers;
            m_outbound_header.msgtype = htonl( payload->msgtype() );
            m_outbound_header.length  = htonl( m_outbound_data.length() );
            buffers.push_back(boost::asio::buffer(&m_outbound_header, sizeof(msg_header)));
            buffers.push_back(boost::asio::buffer(m_outbound_data));
            cout << "writing: "<< payload->toString() << endl;
            boost::asio::async_write(m_socket, buffers, handler);
        }
        else
        {
            cerr << "Failed to serialize" << endl;
        }
    }
    
    /// Asynchronously read a data structure from the socket.
    template <typename FuncTemplate>
    void async_read(FuncTemplate handler)
    {
        // Issue a read operation to read exactly the number of bytes in a header.
        void (Connection::*f)(  const boost::system::error_code&, 
                                boost::tuple<FuncTemplate> )
            = &Connection::handle_read_header<FuncTemplate>;
        
        boost::asio::async_read(m_socket, 
                                boost::asio::buffer((char*)&m_inbound_header,
                                sizeof(msg_header)),
                                //boost::asio::transfer_all(),
                                boost::bind(f,
                                            shared_from_this(), 
                                            boost::asio::placeholders::error, 
                                            boost::make_tuple(handler)));
    }
    
    /// Handle a completed read of a message header. The handler is passed using
    /// a tuple since boost::bind seems to have trouble binding a function object
    /// created using boost::bind as a parameter.
    template <typename FuncTemplate>
    void handle_read_header(const boost::system::error_code& e,       
                            boost::tuple<FuncTemplate> handler)
    {

        if (e)
        {
            cerr << "err" << endl;
            msg_ptr null_msg;
            boost::get<0>(handler)(e, null_msg);
        }
        else
        {
            // convert from nbo to host bo
            m_inbound_header.length  = ntohl( m_inbound_header.length );
            m_inbound_header.msgtype = ntohl( m_inbound_header.msgtype );
        // Determine the length of the serialized data.
            cout << "handle_read_header(msgtype="
                << m_inbound_header.msgtype 
                <<", length="<<m_inbound_header.length
                <<")" << endl;
                
        if (m_inbound_header.length == 0 || m_inbound_header.length > 16384)
        {
            // Header doesn't seem to be valid. Inform the caller.
            cerr << "Invalid header" << endl;
            boost::system::error_code error(boost::asio::error::invalid_argument);
            msg_ptr null_msg;
            boost::get<0>(handler)(error,null_msg);
            return;
        }
            
        // Start an asynchronous call to receive the data.
        void (Connection::*f)(
            const boost::system::error_code&,
            boost::tuple<FuncTemplate>)
            = &Connection::handle_read_data<FuncTemplate>;
    
        //cout << "read_All for " << m_inbound_header.length << "bytes" << endl;
        boost::asio::async_read(m_socket, 
                                boost::asio::buffer((void*)&m_inbound_data,
                                          (size_t)m_inbound_header.length),
                                boost::bind(f,  
                                            shared_from_this(),
                                            boost::asio::placeholders::error, 
                                            handler));
        }
    }
    
    /// Handle a completed read of message data.
    template <typename FuncTemplate>
    void handle_read_data(const boost::system::error_code& e,
                          boost::tuple<FuncTemplate> handler)
    {
        cout << "handle_read_data" << endl;
        if (e)
        {
            cerr << "errrrrrr" << endl;
            msg_ptr null_msg;
            boost::get<0>(handler)(e,null_msg);
        }
        else
        {
            // Extract the data structure from the data just received.
            msg_ptr msg;
            try
            {
                string payload = string(m_inbound_data, m_inbound_header.length);
                cout << "handle_read_data("<< payload <<")" << endl;
                msg = LameMsg::unserialize(m_inbound_header, payload);
                cout << "msg.tostring: " << msg->toString() << endl;
            }
            catch (std::exception& e)
            {
                // Unable to decode data.
                boost::system::error_code error(boost::asio::error::invalid_argument);
                msg_ptr null_msg;
                boost::get<0>(handler)(error, null_msg);
                return;
            }
            // Inform caller that data has been received ok.
            boost::get<0>(handler)(e,msg);
        }
    }

    string username() const { return m_username; }
    void set_username(string u){ m_username =  u; }
    bool authed() const { return m_authed; }
    void set_authed(bool b) { m_authed = b; }
    
private:
    /// The underlying socket.
    boost::asio::ip::tcp::socket m_socket;
    
    msg_header m_outbound_header;
    std::string m_outbound_data;
    
    msg_header m_inbound_header;
    char m_inbound_data[16384]; // 16k buffer max!
    
    /// Statefull stuff the protocol handler/servent will set:
    string m_username; // username of user at end of Connection
    bool m_authed;

};



} // namespace darknet
} // namespace playdar

#endif 
