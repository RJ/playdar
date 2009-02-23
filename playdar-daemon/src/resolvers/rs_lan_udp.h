#ifndef __RS_LAN_UDP_H__
#define __RS_LAN_UDP_H__
#include "resolvers/resolver_service.h"
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "boost/bind.hpp"

#include "application/types.h"

class RS_lan_udp : public ResolverService
{
    public:
    RS_lan_udp(MyApplication * a);
    ~RS_lan_udp();
    void init();
    void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    std::string name() { return "LAN/UDP"; }
    
    void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd);
    void start_listening(boost::asio::io_service& io_service,
        const boost::asio::ip::address& listen_address,
        const boost::asio::ip::address& multicast_address,
        const short multicast_port);
        
  //  private:

    boost::asio::ip::udp::socket * socket_;
    boost::asio::ip::udp::endpoint sender_endpoint_;
    enum { max_length = 1024 };
    char data_[max_length];
};

#endif
