#ifndef __RS_LAN_UDP_DL_H__
#define __RS_LAN_UDP_DL_H__

#include "resolvers/resolver_service.h"

#include <iostream>
#include <map>

#include <string>
#include <boost/asio.hpp>
#include "boost/bind.hpp"
#include <DynamicClass.hpp>
#include "application/types.h"

/*
    Broadcast search queries on the LAN using UDP multicast
    Works by sending JSON-serialized ResolverQuery objects.
    
    Responses come in via UDP, and we stream songs using HTTP.
*/

namespace playdar {
namespace resolvers {

class lan_udp : public ResolverService
{
    public:
    lan_udp(){}
    
    void init(playdar::Config * c, MyApplication * a);
    void run();
    void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    std::string name() { return "LAN/UDP"; }
    
    void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd);
    void start_listening(boost::asio::io_service& io_service,
        const boost::asio::ip::address& listen_address,
        const boost::asio::ip::address& multicast_address,
        const short multicast_port);
    
    void send_response( query_uid qid, 
                        boost::shared_ptr<PlayableItem> pip,
                        boost::asio::ip::udp::endpoint sep );
                        
protected:    
    ~lan_udp() throw();
    
private:

    void handle_send(   const boost::system::error_code& error,
                                size_t bytes_recvd,
                                char * scratch );

    void async_send(boost::asio::ip::udp::endpoint * remote_endpoint,
                    string message);

    boost::asio::ip::udp::socket * socket_;
    boost::asio::ip::udp::endpoint sender_endpoint_;
    boost::asio::ip::udp::endpoint * broadcast_endpoint_;
    enum { max_length = 1024 };
    char data_[max_length];
};

EXPORT_DYNAMIC_CLASS( lan_udp )


}}
#endif
