#ifndef __LOCAL_FILE_STRAT_H__
#define __LOCAL_FILE_STRAT_H__

#include <fstream>  
#include <sstream>
#include <iostream>

#include <boost/filesystem.hpp> 

namespace playdar {

class LocalFileStreamingStrategy : public StreamingStrategy
{
public:

    LocalFileStreamingStrategy(const std::string& p)
        : m_uri(p)
    {
        m_connected=false;
    }
    
    ~LocalFileStreamingStrategy()
    {
        reset();
    }
    
    int read_bytes(char * buf, size_t size)
    {
        if(!m_connected) do_connect();
        if(!m_connected) return 0;
        int len = m_is.read(buf, size).gcount();
        if(len==0) reset();
        return m_is.gcount();
    }
    
    std::string debug()
    { 
        std::ostringstream s;
        s << "LocalFileStreamingStrategy(" << m_uri << ")";
        return s.str();
    }

    void reset()
    {
        if(m_is.is_open()) m_is.close();
        m_connected = false;
    }
    
private:

    void do_connect()
    {
        reset();
        //cout << debug() << endl;
        m_is.open(m_uri.c_str(), std::ios::in | std::ios::binary);
        if(!m_is.is_open())
        {
            std::cout << "Failed to open file: " << m_uri << std::endl;
            return;
        }
        m_connected = true;
    }
    
    std::string m_uri;
    std::ifstream m_is;
    bool m_connected;
};

}

#endif
