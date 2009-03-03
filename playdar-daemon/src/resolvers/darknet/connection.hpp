#ifndef _PLAYDAR_DARKNET_CONNECTION_HPP_
#define _PLAYDAR_DARKNET_CONNECTION_HPP_

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
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
        : m_socket(io_service), m_sending(false), m_authed(false)
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
    /// Seems like asio doesnt make any promises if you stack up multiple
    /// async_write calls - so we queue them and call it sequentially.
    void async_write(msg_ptr msg)//, FuncTemplate handler)
    {
        {
            boost::mutex::scoped_lock lk(m_mutex);
            m_writeq.push_back(msg);
        }
        msg_ptr noop(new LameMsg(GENERIC));
        boost::system::error_code e;
        do_async_write(e, noop);
    }
    
    /// Calls to do_async_write are chained - it will call itself when a write
    /// completes, to send the next msg in the queue. Bails when none left.
    void do_async_write(const boost::system::error_code& e, msg_ptr finished_msg)
    {
        boost::mutex::scoped_lock lk(m_mutex);
        // if this is a GENERIC (ie, null) msg, it's actually a dispatch call:
        if(m_sending && finished_msg->msgtype()==GENERIC)
        {
            //cout << "bailing from do_async_write - already sending (sending=true)" << endl;
            return;
        }
        if(m_writeq.empty())
        {
            //cout << "bailing from do_async_write, q empty (sending=false)" << endl;
            m_sending = false;
            return;
        }
        msg_ptr msg = m_writeq.front();
        m_writeq.pop_front();
        msg_header outbound_header;
        outbound_header.msgtype = htonl( msg->msgtype() );
        outbound_header.length  = htonl( msg->payload_len() );
        size_t data_len = sizeof(msg_header) + msg->payload_len();
        char * data = (char*) malloc(data_len);
        memcpy(data, (char*)&outbound_header, sizeof(msg_header));
        memcpy(data+sizeof(msg_header), msg->serialize().data(), msg->payload_len()); 
        msg->set_membuf(data); // msg will free() data on destruction.
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(data, data_len));
        // add to write queue:
        m_sending = true;
        cout << "dispatching write. write queue size is: " << m_writeq.size() << endl;
        boost::asio::async_write(m_socket, buffers, 
                                 boost::bind(&Connection::do_async_write, this,
                                      boost::asio::placeholders::error, 
                                      msg)); 
    }

    
    
    /// Setup a call to read the next msg_header
    template <typename FuncTemplate>
    void async_read(FuncTemplate handler)
    {
        msg_ptr msg(new LameMsg()); // what the data from the wire becomes
        void (Connection::*f)(  const boost::system::error_code&, 
                                boost::tuple<FuncTemplate>,
                                msg_ptr,
                                char *)
            = &Connection::handle_read_header<FuncTemplate>;
        // Issue a read operation to read exactly the number of bytes in a header.
        char * header_buf = (char*) malloc(sizeof(msg_header)*sizeof(char));
        boost::asio::async_read(m_socket, 
                                boost::asio::buffer(header_buf, 
                                                    sizeof(msg_header)),
                                boost::bind(f,
                                            shared_from_this(), 
                                            boost::asio::placeholders::error, 
                                            boost::make_tuple(handler),
                                            msg,
                                            header_buf));
    }
    
    
    /// Handle a completed read of a message header. The handler is passed using
    /// a tuple since boost::bind seems to have trouble binding a function object
    /// created using boost::bind as a parameter.
    template <typename FuncTemplate>
    void handle_read_header(const boost::system::error_code& e,       
                            boost::tuple<FuncTemplate> handler,
                            msg_ptr msg,
                            char * header_buf)
    {
        //cout << "handle_read_header" << endl;
        
        if (e)
        {
            cerr << "err" << endl;
            free(header_buf);
            msg_ptr null_msg;
            boost::get<0>(handler)(e, null_msg);
            return;
        }
        msg_header * inbound_header = (msg_header *)header_buf;
        // convert from nbo to host bo
        inbound_header->length  = ntohl( inbound_header->length );
        inbound_header->msgtype = ntohl( inbound_header->msgtype );
        
        msg->m_msgtype = inbound_header->msgtype;
        msg->m_expected_len = inbound_header->length;
        

        
        // Start an asynchronous call to receive the data.
        void (Connection::*f)(
            const boost::system::error_code&,
            boost::tuple<FuncTemplate>,
            msg_ptr,
            char *)
            = &Connection::handle_read_data<FuncTemplate>;
    
        cout    << "header, msgtype = " << inbound_header->msgtype
                << " length = " << inbound_header->length 
                << " read.. " << inbound_header->length << " bytes" << endl;
        // 16K is max payload size.
        assert( msg->m_expected_len <= 16384 );
        
        size_t payloadsize = inbound_header->length * sizeof(char);
        char * payload_buf = (char*) malloc(payloadsize);
                
        assert(inbound_header->length <= payloadsize);
        
        free(header_buf);
        
        boost::asio::async_read(m_socket, 
                                boost::asio::buffer(payload_buf, payloadsize),
                                boost::bind(f,  
                                            shared_from_this(),
                                            boost::asio::placeholders::error, 
                                            handler,
                                            msg,
                                            payload_buf));
        
    }
    
    /// Handle a completed read of message data.
    template <typename FuncTemplate>
    void handle_read_data(const boost::system::error_code& e,
                          boost::tuple<FuncTemplate> handler,
                          msg_ptr msg,
                          char * payload_buf)
    {
        //cout << "handle_read_data" << endl;
        if (e)
        {
            free(payload_buf);
            cerr << "errrrrrr" << endl;
            boost::get<0>(handler)(e,msg);
        }
        else
        {
            // build the lamemsg structure from the data just received.
            try
            {
                msg->set_payload(string((const char *)payload_buf, msg->m_expected_len), msg->m_expected_len);
                free(payload_buf);
                cout << "handle_read_data("<< msg->toString() <<")" << endl;
            }
            catch (std::exception& e)
            {
                // Unable to decode data.
                boost::system::error_code error(boost::asio::error::invalid_argument);
                boost::get<0>(handler)(error, msg);
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
    
    boost::mutex m_mutex;
    bool m_sending; // are we currently awaiting an async_write to finish
    deque< msg_ptr > m_writeq; // queue of outgoing messages
    
    /// Stateful stuff the protocol handler/servent will set:
    string m_username; // username of user at end of Connection
    bool m_authed;
    
};



} // namespace darknet
} // namespace playdar

#endif 
