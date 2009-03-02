#ifndef __DARKNET_STREAMING_STRAT_H__
#define __DARKNET_STREAMING_STRAT_H__

#include "resolvers/darknet/msgs.h"
#include "resolvers/darknet/servent.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

using namespace playdar::darknet;

class DarknetStreamingStrategy : public StreamingStrategy
{
public:

    DarknetStreamingStrategy(RS_darknet * darknet, connection_ptr conn, source_uid sid, int size)
        : m_conn(conn), m_sid(sid)
    {
        m_darknet = darknet;
        m_filesize = size;
        m_nextpart = 0;
        m_partoffset = 0;
        m_partposition = 0;
        m_active = false;
    }
    
    ~DarknetStreamingStrategy() {}
    
    
    int read_bytes(char * buf, int size)
    {
        if(!m_active) start(); // tell peer to start sending us data.
        boost::mutex::scoped_lock lk(m_mutex);
        // Wait until current part available:
        while( m_partposition >= m_nextpart )
        {
            cout << "Waiting on part " << m_partposition << "..." << endl;
            m_cond.wait(lk);
        }
        cout << "got part "<<m_partposition << endl;
        // if this msg has 0 data, it signifies end of stream.
        if(m_parts[m_partposition]->payload().length() == sizeof(sid_header))
        {
            cout << "End of stream marker reached." << endl;
            return 0;
        }
        
        const char * partleftstart = m_parts[m_partposition]->payload().c_str();
        partleftstart += sizeof(sid_header) + m_partoffset;
        int partleftsize = m_parts[m_partposition]->payload().length() - sizeof(sid_header) - m_partoffset;
        cout << "partleftsize: " << partleftsize << endl;
        if(partleftsize >= size)
        {
            memcpy(buf, partleftstart, size);
            m_partoffset += size;
            //m_cond.notify_one();
            return size;
        }
        else // handle part rollover case
        {
            memcpy(buf, partleftstart, partleftsize);
            m_partposition++;
            m_partoffset = 0;
            //m_cond.notify_one();
            // make them re-request to get data from next part:
            return partleftsize; 
        }
    }
    
    string debug()
    { 
        ostringstream s;
        s<< "DarknetStreamingStrategy(darknet :: " << m_conn->username() << " :: " << m_sid << ")";
        return s.str();
    }
    
    void reset() {}
    
    void start()
    {
        m_active = true;
        m_darknet->start_sidrequest(m_conn, m_sid, 
                             boost::bind(&DarknetStreamingStrategy::siddata_handler, this, _1));
         
    }
    
    bool siddata_handler(msg_ptr msg)
    {
        boost::mutex::scoped_lock lk(m_mutex);
        cout << "SSDarknet siddata handler, got part." << endl;
        // first part of payload is sid_header:
        sid_header sheader;
        memcpy(&sheader, msg->payload().c_str(), sizeof(sid_header));
        source_uid sid = string((char *)&sheader.sid, 36);
        if(m_parts.size()==0)
        {
            cout << "First part arrived, setting up." << endl;
            //m_parts.resize(sheader.numparts);
        }
        m_parts.push_back(msg);
        m_nextpart++;
        m_cond.notify_one();
        return true;
    } 
    
    
private:
    RS_darknet * m_darknet;
    connection_ptr m_conn;
    source_uid m_sid;
    int m_filesize; 
    int m_nextpart;
    vector<msg_ptr> m_parts;

    boost::mutex m_mutex;
    boost::condition m_cond;


    bool m_active;
    // keeps track of how much data we've read so far:
    int m_partposition;
    int m_partoffset;
};

#endif
