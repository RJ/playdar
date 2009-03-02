#ifndef __DARKNET_STREAMING_STRAT_H__
#define __DARKNET_STREAMING_STRAT_H__

#include "resolvers/darknet/msgs.h"
#include "resolvers/darknet/servent.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <deque>

using namespace playdar::darknet;

class DarknetStreamingStrategy : public StreamingStrategy
{
public:

    DarknetStreamingStrategy(RS_darknet * darknet, 
                             connection_ptr conn, 
                             source_uid sid)
        : m_conn(conn), m_sid(sid)
    {
        m_darknet = darknet;
        reset();
    }
    
    ~DarknetStreamingStrategy() {}
    
    
    int read_bytes(char * buf, int size)
    {
        if(!m_active) start();
        boost::mutex::scoped_lock lk(m_mutex);
        // Wait until data available:
        while( (m_data.empty() || m_data.size()==0) && !m_finished )
        {
            cout << "Waiting for SIDDATA message..." << endl;
            m_cond.wait(lk);
        }
        //cout << "Got data to send, max available: "<< m_data.size() << endl;
        if(!m_data.empty())
        {
            int sent = 0;
            for(; !m_data.empty() && (sent < size); sent++)
            {
                *(buf+sent) = m_data.front();
                m_data.pop_front();
            }
            if(sent) return sent;
        }
        else if(m_finished)
        {
            cout << "End of stream marker reached. "
                 << m_numrcvd << " bytes rcvd total" << endl;
            cout << "Current size of output buffer: " << m_data.size() << endl;
            assert(m_data.size()==0);
            return 0;
        }
        else
        {
            assert(0); // wtf.
            return 0;
        }
    }
    
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
    void start()
    {
        reset();
        m_active = true;
        m_darknet->start_sidrequest(
            m_conn, 
            m_sid, 
            boost::bind(&DarknetStreamingStrategy::siddata_handler, 
                        this, _1)
        ); 
    }
    
    bool siddata_handler(msg_ptr msg)
    {
        boost::mutex::scoped_lock lk(m_mutex);
        //cout << "SSDarknet siddata handler, recv before this handler fired: "
        //     << m_numrcvd << endl;
        // first part of payload is sid_header:
        sid_header sheader;
        memcpy(&sheader, msg->payload().data(), sizeof(sid_header));
        source_uid sid = string((char *)&sheader.sid, 36);

        /* 
        // hack to write out to a file, too:
        if(msg->payload_len() > sizeof(sid_header))
        {
            string fname = "/tmp/playdar.";
            fname += sid;
            ofstream tmp(fname.c_str(), ios::out | ios::app);
            tmp.write(msg->payload().data() + sizeof(sid_header),
                    msg->payload_len() - sizeof(sid_header));
            tmp.close();
        }
        */
        int toread = (msg->payload_len()-sizeof(sid_header));
        assert(toread >= 0);
        if(toread == 0)
        {
            cout << "last SIDDATA msg has arrived ("
                 << m_numrcvd <<" bytes total)." << endl;
            m_finished = true;
        }
        else
        {
            // copy data to the output queue
            for(int k = 0; k< toread; ++k)
            {
                m_data.push_back( *(msg->payload().data() 
                                    + sizeof(sid_header)+k) );
                m_numrcvd++;
            }
        }
        m_cond.notify_one();
        return true;
    }
    
    
private:
    RS_darknet * m_darknet;
    connection_ptr m_conn;
    source_uid m_sid;

    boost::mutex m_mutex;
    boost::condition m_cond;

    deque< char > m_data;   //output buffer
    unsigned int m_numrcvd; //bytes recvd so far
    bool m_finished;        //EOS reached?

    bool m_active;
};

#endif
