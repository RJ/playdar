#ifndef __DARKNET_STREAMING_STRAT_H__
#define __DARKNET_STREAMING_STRAT_H__

#include "playdar/streaming_strategy.h"

#include "msgs.h"
#include "servent.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <deque>

namespace playdar {
namespace resolvers {
    
class DarknetStreamingStrategy : public StreamingStrategy
{
public:

    DarknetStreamingStrategy(darknet * darknet, 
                             connection_ptr conn, 
                             std::string sid);
    
    ~DarknetStreamingStrategy() ;
    
    
    static boost::shared_ptr<DarknetStreamingStrategy> factory(std::string url, darknet * darknet);
    
    
    size_t read_bytes(char * buf, size_t size);
    
    string debug()
    { 
        ostringstream s;
        s<< "DarknetStreamingStrategy(from:" << m_conn->username() << " sid:" << m_sid << ")";
        return s.str();
    }
    
    void reset() 
    {
        m_active = false;
        m_finished = false;
        m_numrcvd=0;
        m_data.clear();
    }
    
     // tell peer to start sending us data.
    void start();
    
    bool siddata_handler(msg_ptr msg);

    std::string mime_type() { return "dont/know"; }
    
private:
    darknet * m_darknet;
    connection_ptr m_conn;
    std::string m_sid;

    boost::mutex m_mutex;
    boost::condition m_cond;

    deque< char > m_data;   //output buffer
    unsigned int m_numrcvd; //bytes recvd so far
    bool m_finished;        //EOS reached?

    bool m_active;
};

}}

#endif
