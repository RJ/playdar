#ifndef __RS_lan_DL_H__
#define __RS_lan_DL_H__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <map>
#include <string>

#include "playdar/playdar_plugin_include.h"


/*
    Broadcast search queries on the LAN using UDP multicast
    Works by sending JSON-serialized ResolverQuery objects.
    
    Responses come in via UDP, and we stream songs using HTTP.
*/

namespace playdar {
namespace resolvers {

class lan : public ResolverPlugin<lan>
{
    public:
    lan(): socket_( 0 ),
           broadcast_endpoint_( 0 ){}
    
    virtual bool init(pa_ptr pap);

    void run();
    void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    void cancel_query(query_uid qid);
    
    std::string name() const { return "LAN"; }
    
    void handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd);
    void start_listening(boost::asio::io_service& io_service,
        const boost::asio::ip::address& listen_address,
        const boost::asio::ip::address& multicast_address,
        const short multicast_port);
    
    void send_response( query_uid qid, 
                        ri_ptr rip,
                        boost::asio::ip::udp::endpoint sep );
    
    /// max time in milliseconds we'd expect to have results in.
    unsigned int target_time() const
    {
        return 50;
    }
    
    /// highest weighted resolverservices are queried first.
    unsigned short weight() const
    {
        return 99;
    }
        
    playdar_response http_handler( const playdar_request& req,
                         playdar::auth * pauth);
    
protected:    
    virtual ~lan() throw();
    
private:

    pa_ptr m_pap;

    boost::shared_ptr< boost::asio::io_service > m_io_service;
    boost::shared_ptr< boost::thread > m_responder_thread;
    
    //boost::asio::io_service * m_io_service;
    // boost::thread * m_responder_thread;

    void handle_send( const boost::system::error_code& error,
                      size_t bytes_recvd,
                      char * scratch );

    void async_send( boost::asio::ip::udp::endpoint * remote_endpoint,
                     std::string message );

    boost::asio::ip::udp::socket * socket_;
    boost::asio::ip::udp::endpoint sender_endpoint_;
    boost::asio::ip::udp::endpoint * broadcast_endpoint_;
    enum { max_length = 1024 };
    char data_[max_length];
    
    // a lan node we got a ping from:
    struct lannode
    { 
        std::string name;
        time_t lastdate; 
        std::string http_base;
        boost::asio::ip::udp::endpoint udp_ep;
    };

    // nodes we've seen:
    std::map<std::string,lannode> m_lannodes;

    // lan discovery:
    void send_ping();
    void send_pong( boost::asio::ip::udp::endpoint sender_endpoint);
    void send_pang();

    void receive_pong(std::map<std::string, json_spirit::Value> & om,
                      const boost::asio::ip::udp::endpoint &  sender_endpoint);
    void receive_ping(std::map<std::string, json_spirit::Value> & om,
                      const boost::asio::ip::udp::endpoint &  sender_endpoint);
    void receive_pang(std::map<std::string, json_spirit::Value> & om,
                      const boost::asio::ip::udp::endpoint &  sender_endpoint);
};

EXPORT_DYNAMIC_CLASS( lan )


}}
#endif
