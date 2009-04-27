#include "servent.h"
#include "darknet.h"

using namespace std;

namespace playdar {
namespace resolvers {

Servent::Servent(boost::asio::io_service& io_service, 
        unsigned short port, 
        darknet * p ) //boost::shared_ptr<darknet> p)
    :   m_acceptor(io_service,
                   boost::asio::ip::tcp::endpoint
                        (boost::asio::ip::tcp::v4(), port)),
        m_protocol(p)
{
    //p->init(this); // configure protocol.
    
    // Start an accept operation for a new connection.
    connection_ptr new_conn(new Connection(m_acceptor.io_service()));

    m_acceptor.async_accept(new_conn->socket(),
        boost::bind(&Servent::handle_accept, this,
        boost::asio::placeholders::error, new_conn));
}

/// Handle completion of a accept operation.
void 
Servent::handle_accept(const boost::system::error_code& e, connection_ptr conn)
{
    if (!e)
    {
        if(m_protocol->new_incoming_connection(conn))
        {
            conn->async_read(   
                boost::bind( &Servent::handle_read, this, _1, _2, conn )
            );
        }
        
        // Start an accept operation for a new connection.
        connection_ptr new_conn(new Connection(m_acceptor.io_service()));
        m_acceptor.async_accept(new_conn->socket(),
            boost::bind(&Servent::handle_accept, this,
                boost::asio::placeholders::error, new_conn));
    }
    else
    {
        // An error occurred. Log it and return. Since we are not starting a new
        // accept operation the io_service will run out of work to do and the
        // Servent will exit.
        std::cerr << e.message() << std::endl;
    }
}
/*
/// Handle completion of a write operation.
void 
Servent::handle_write(  const boost::system::error_code& e, 
                    connection_ptr conn,
                    msg_ptr msg)
{
    m_protocol->write_completed(conn, msg);
}
*/
/// Handle completion of a read operation.
void 
Servent::handle_read(   const boost::system::error_code& e, 
                    msg_ptr msg, 
                    connection_ptr conn)
{
    if(!m_protocol->handle_read(e, msg, conn)) return;
    
    //cout << "..." << endl;
    conn->async_read(   
        boost::bind( &Servent::handle_read, this, _1, _2, conn )
    );
}



/// Connect out to a remote Servent:
void 
Servent::connect_to_remote(boost::asio::ip::tcp::endpoint &endpoint)
{
    cout << "connect_to_remote("<<endpoint.address().to_string()<<","<<endpoint.port()<<")"<<endl;
    connection_ptr new_conn(new Connection(m_acceptor.io_service()));
    // Start an asynchronous connect operation.
    new_conn->socket().async_connect(endpoint,
        boost::bind(&Servent::handle_connect, this,
        boost::asio::placeholders::error, endpoint, new_conn));
}

/// Handle completion of a connect operation.
void 
Servent::handle_connect(const boost::system::error_code& e,
                    boost::asio::ip::tcp::endpoint &endpoint,
                    connection_ptr conn)
{
    if (!e)
    {
        /// Successfully established connection. 
        m_protocol->new_outgoing_connection(conn, endpoint);
        
        conn->async_read(   
            boost::bind( &Servent::handle_read, this, _1, _2, conn )
        );
    }
    else
    {
        std::cerr   << "Failed to connect out to remote Servent: " 
                    << e.message() << std::endl;
    }
}

}} //namepsaces
