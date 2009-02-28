#ifndef __DARKNET_STREAMING_STRAT_H__
#define __DARKNET_STREAMING_STRAT_H__

#include "resolvers/darknet/msgs.h"
#include "resolvers/darknet/servent.h"

using namespace playdar::darknet;

class DarknetStreamingStrategy : public StreamingStrategy
{
public:

    DarknetStreamingStrategy(connection_ptr conn, source_uid sid)
        : m_conn(conn), m_sid(sid)
    {

    }
    
    ~DarknetStreamingStrategy() {}
    
    
    int read_bytes(char * buf, int size)
    {
        /*if(!m_connected) do_connect();
        if(!m_connected) return 0;
        int len = m_is.read(buf, size).gcount();
        if(len==0) reset();
        return m_is.gcount();*/
        return 0;
    }
    
    string debug()
    { 
        ostringstream s;
        s<< "DarknetStreamingStrategy(darknet :: " << m_conn->username() << " :: " << m_sid << ")";
        return s.str();
    }
    
    void reset() {}

    
private:

    connection_ptr m_conn;
    source_uid m_sid;
};

#endif
