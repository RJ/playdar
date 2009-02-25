#include <iostream>
#include <sstream>
#include <string>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "boost/bind.hpp"
#include "boost/date_time/posix_time/posix_time_types.hpp"

using namespace std;

// this can be removed now - 
// we already bound a port for listening, should use it for sending too.

class UDPSender
{
    public:
    static void send(boost::asio::ip::address addr, unsigned short port, string message)
    {
        try
        {
            //cout << "Sending UDP ("<<message.length()<<"bytes): " << endl << message << endl;
            boost::asio::io_service io_service;
            UDPSender s(io_service, addr, port, message);
            io_service.run(); 
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }

    UDPSender(boost::asio::io_service& io_service,
                const boost::asio::ip::address& multicast_address,
                unsigned short port,
                string message)
            : endpoint_(multicast_address, port),
            socket_(io_service, endpoint_.protocol())
    {
        socket_.async_send_to(
                boost::asio::buffer(message), endpoint_,
                boost::bind(&UDPSender::handle_send_to, this,
                    boost::asio::placeholders::error));
    }

    void handle_send_to(const boost::system::error_code& error)
    {
        if (error)
        {
            cerr << "Error sending udp msg." << endl;
        }
        else
        {
            //cout << "send udp msg" << endl;
        }
    }

    private:
        boost::asio::ip::udp::endpoint endpoint_;
        boost::asio::ip::udp::socket socket_;
};


