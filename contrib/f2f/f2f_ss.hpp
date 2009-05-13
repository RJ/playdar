#ifndef __F2F_SS_H__
#define __F2F_SS_H__

#include <string>
#include <cstdio>


#include "playdar/streaming_strategy.h"

namespace playdar {

class f2fStreamingStrategy : public StreamingStrategy
{
public:
    f2fStreamingStrategy(std::string url, boost::shared_ptr<jbot> j)
        : m_url(url), m_j(j)
    {
        if(!j)
        {
            std::cout <<"No jbot inst." << std::endl;
            throw;
        }
        if( url.substr(0,6) != "f2f://" ) throw;
        size_t hashpos = url.find("#");
        if( hashpos == std::string::npos ) throw;
        m_jid = url.substr(6,hashpos-6);
        m_sid = url.substr(hashpos+1);
        std::cout << "CTOR f2fSS jid='"<<m_jid<<"' sid='"<<m_sid<<"'" << std::endl;
    }
    
    ~f2fStreamingStrategy(){}
    
    static boost::shared_ptr<f2fStreamingStrategy> factory(std::string url, boost::shared_ptr<jbot> j)
    {
        return boost::shared_ptr<f2fStreamingStrategy>(new f2fStreamingStrategy(url,j));
    }
    
    std::string debug() { return m_url; }
    void reset() {}
    std::string mime_type() { return "dont/know"; }
    
    size_t read_bytes(char * buf, size_t size)
    {
        // TODO
        return 0;
    }
    

private:
    std::string m_url, m_jid, m_sid;
    boost::shared_ptr<jbot> m_j;
};

} //ns
#endif
