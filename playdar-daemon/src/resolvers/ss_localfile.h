#ifndef __LOCAL_FILE_STRAT_H__
#define __LOCAL_FILE_STRAT_H__
#include "boost/filesystem.hpp" 
#include <fstream>  

class LocalFileStreamingStrategy : public StreamingStrategy
{
public:

    LocalFileStreamingStrategy(string p)
        : m_uri(p)
    {
        m_connected=false;
    }
    
    ~LocalFileStreamingStrategy()
    {
        reset();
    }
    
    int read_bytes(char * buf, int size)
    {
        if(!m_connected) do_connect();
        if(!m_connected) return 0;
        int len = m_is.read(buf, size).gcount();
        if(len==0) reset();
        return m_is.gcount();
    }
    
    string debug()
    { 
        ostringstream s;
        s<< "LocalFileStreamingStrategy(" << m_uri << ")";
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
            cout << "Failed to open file: " << m_uri << endl;
            return;
        }
        m_connected = true;
    }
    
    string m_uri;
    std::ifstream m_is;
    bool m_connected;
};

#endif
