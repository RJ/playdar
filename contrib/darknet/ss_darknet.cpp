#include "ss_darknet.h"

#include "playdar/types.h"
#include "darknet.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <deque>

namespace playdar {
namespace resolvers {
        
        
DarknetStreamingStrategy::DarknetStreamingStrategy(darknet* darknet, connection_ptr conn, std::string sid)
    : m_conn(conn), m_sid(sid)
{
    m_darknet = darknet;
    reset();
}


DarknetStreamingStrategy::~DarknetStreamingStrategy()
{
    cout << "DTOR " << debug() << endl;
}

boost::shared_ptr<DarknetStreamingStrategy> 
DarknetStreamingStrategy::factory(std::string url, playdar::resolvers::darknet* darknet)
{
    cout << "in DSS::factory with url:" << url << endl;
    size_t offset = url.find(":");
    if( offset == string::npos ) return boost::shared_ptr<DarknetStreamingStrategy>();
    size_t offset2 = url.find( "/sid/" );
    if( offset2 == string::npos ) return boost::shared_ptr<DarknetStreamingStrategy>();
    string username = url.substr(offset + 2, offset2); // username/sid/<sid>
    string sid = url.substr( offset2 + 5, url.length() );
    
    cout << "got username in darknet_ss::factory:" << username << "and sid:" << sid << endl;
    connection_ptr con = darknet->connections()[username].lock();
    
    return boost::shared_ptr<DarknetStreamingStrategy>(new DarknetStreamingStrategy(darknet, con, sid ) );
}

size_t 
DarknetStreamingStrategy::read_bytes(char* buf, size_t size)
{
    if(!m_active) start();
    boost::mutex::scoped_lock lk(m_mutex);
    // Wait until data available:
    while( (m_data.empty() || m_data.size()==0) && !m_finished )
    {
        //cout << "Waiting for SIDDATA message..." << endl;
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
        assert(0);
        return 0;
    }
    else if(m_finished)
    {
        cout << "End of stream marker reached. "
        << m_numrcvd << " bytes rcvd total" << endl;
        cout << "Current size of output buffer: " << m_data.size() << endl;
        assert(m_data.size()==0);
        reset();
        return 0;
    }
    else
    {
        assert(0); // wtf.
        return 0;
    }
}

void 
DarknetStreamingStrategy::start()
{
    reset();
    if(!m_conn->alive())
    {
        cout << "Darknet connection went away :(" << endl;
        throw;
    }
    m_active = true;
    // setup a timer to abort if we don't get data quick enough?
    m_darknet->start_sidrequest( m_conn, m_sid, 
                                 boost::bind(&DarknetStreamingStrategy::siddata_handler, 
                                             this, _1)
                                             ); 
}

bool 
DarknetStreamingStrategy::siddata_handler(playdar::resolvers::msg_ptr msg)
{
    boost::mutex::scoped_lock lk(m_mutex);
    //cout << "SSDarknet siddata handler, recv before this handler fired: "
    //     << m_numrcvd << endl;
    // first part of payload is sid_header:
    sid_header sheader;
    memcpy(&sheader, msg->payload().data(), sizeof(sid_header));
    std::string sid = string((char *)&sheader.sid, 36);
    
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

}
}