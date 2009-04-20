#ifndef __CURL_STRAT_H__
#define __CURL_STRAT_H__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <curl/curl.h>

#include "playdar/streaming_strategy.h"
#include "playdar/utils/base64.h"

namespace playdar {

using namespace boost::asio::ip;
/*
    Can stream from anything cURL can.. 
    
    TODO handle failure/slow streams better.
    TODO potential efficiency gain by storing data in lumps of size returned by curl
    TODO make some of the curl options user-configurable (ssl cert checking, auth, timeouts..)
*/
class CurlStreamingStrategy : public StreamingStrategy
{
public:
    CurlStreamingStrategy(std::string url)
        : m_curl(0)
        , m_url(url)
        , m_thread(0)
        , m_abort(false)
    {
        std::cout << "CTOR ss_curl: " << url << std::endl;
        reset();
    }
    
    /// copy constructor, used by get_instance()
    CurlStreamingStrategy(const CurlStreamingStrategy& other)
        : m_curl(0)
        , m_url(other.url())
        , m_thread(0)
        , m_abort(false)
    {
        reset();
    }

    ~CurlStreamingStrategy()
    { 
        std::cout << "DTOR ss_curl: " << m_url << std::endl;
        if(m_thread)
        {
            m_abort = true; // will cause thread to terminate
            m_thread->join();
        }
        reset(); 
    }

    /// this returns a shr_ptr to a copy, because it's not threadsafe 
    virtual boost::shared_ptr<StreamingStrategy> get_instance()
    {
        // make a copy:
        return boost::shared_ptr<StreamingStrategy>(new CurlStreamingStrategy(*this));
    }
    
    std::vector<std::string> & extra_headers() { return m_extra_headers; }

    int read_bytes(char * buf, size_t size)
    {
        if(!m_connected) do_connect();
        if(!m_connected)
        {
            std::cout << "ERROR: do_connect failed in httpss." << std::endl;
            if( m_curl ) curl_easy_cleanup( m_curl );
            reset();
            return 0;
        }
        do{
            boost::mutex::scoped_lock lk(m_mut);
            while( m_buffers.size()==0 && !m_curl_finished )
            {
                //cout << "Waiting on curl.." << endl;
                m_cond.wait(lk);
            }
            //cout << "data available: "<< m_buffers.size() << " bytes." << endl;
            if( m_curl_finished && m_buffers.size() == 0 ) break;
            size_t i;
            for( i = 0; i < size && i < m_buffers.size(); i++ )
            {
                char c = m_buffers.back();
                memcpy( buf+i, (char*)&c, 1 );
                m_buffers.pop_back();
            }
            return i;
        }while(false);
        reset();
        return 0; // means no more data.
    }

    
    std::string debug()
    { 
        std::ostringstream s;
        s << "CurlStreamingStrategy( " << m_url << " )";
        return s.str();
    }
    
    void reset()
    {
        m_connected = false;
        m_bytesreceived = 0;
        m_curl_finished = false;
        m_abort = false;
        m_buffers.clear();
    }
    
    /// curl callback when data from fetching an url has arrived
    static size_t curl_writefunc( void *vptr, size_t size, size_t nmemb, void *custom )
    {
        CurlStreamingStrategy * inst = ((CurlStreamingStrategy*)custom);
        if( !inst->m_connected ) return 1; // someone called reset(), abort transfer
        char * ptr = (char*) vptr;
        size_t len = size * nmemb;
        inst->m_bytesreceived += len;
        {
            boost::mutex::scoped_lock lk(inst->m_mut);
            for(size_t s = 0; s < len; s++)
            {
                char c = *(ptr+s);
                inst->m_buffers.push_front( c );
            }
        }
        inst->m_cond.notify_all();
        return len;
    }
    
    static int curl_progressfunc(void *clientp/*this instance*/, double dltotal, double dlnow, 
                                 double ultotal, double ulnow)
    {
        //cout << "Curl has downloaded: " << dlnow << " : " << dltotal << endl;
        if( ((CurlStreamingStrategy*)clientp)->m_abort )
        {
            std::cout << "Aborting in-progress download." << std::endl;
            return 1; // non-zero aborts download.
        }
        else
        {
            return 0;
        }
    }
    
    /// run in a thread to do the curl transfer
    void curl_perform()
    {
        std::cout << "doing curl_perform for '" << m_url << "'" << std::endl;
        // this blocks until transfer complete / error:
        m_curlres = curl_easy_perform( m_curl );
        m_curl_finished = true;
        std::cout << "curl_perform done. ret: " << m_curlres << std::endl;
        if(m_curlres != 0) std::cout << "Curl error: " << m_curlerror << std::endl;
        curl_easy_cleanup( m_curl );
        m_cond.notify_all();
    }
    
    const std::string url() const { return m_url; }
    
protected:

    void do_connect()
    {
        if(m_thread)
        {
            m_thread->join();
        }
        std::cout << debug() << std::endl; 
        reset();
        m_curl = curl_easy_init();
        if(!m_curl)
        {
            std::cout << "Curl init failed" << std::endl;
            throw;
        }
        // for curl options, see:
        // http://curl.netmirror.org/libcurl/c/curl_easy_setopt.html
        
        curl_easy_setopt( m_curl, CURLOPT_NOSIGNAL, 1 );
        curl_easy_setopt( m_curl, CURLOPT_CONNECTTIMEOUT, 5 );
        curl_easy_setopt( m_curl, CURLOPT_SSL_VERIFYPEER, 0 );
        curl_easy_setopt( m_curl, CURLOPT_FTP_RESPONSE_TIMEOUT, 10 );
        curl_easy_setopt( m_curl, CURLOPT_URL, m_url.c_str() );
        curl_easy_setopt( m_curl, CURLOPT_FOLLOWLOCATION, 1 );
        curl_easy_setopt( m_curl, CURLOPT_MAXREDIRS, 5 );
        curl_easy_setopt( m_curl, CURLOPT_USERAGENT, "Playdar (libcurl)" );
        curl_easy_setopt( m_curl, CURLOPT_WRITEFUNCTION, &CurlStreamingStrategy::curl_writefunc );
        curl_easy_setopt( m_curl, CURLOPT_WRITEDATA, this );
        curl_easy_setopt( m_curl, CURLOPT_ERRORBUFFER, (char*)&m_curlerror );
        // we use the curl progress callbacks to abort transfers mid-download on exit
        curl_easy_setopt( m_curl, CURLOPT_NOPROGRESS, 0 );
        curl_easy_setopt( m_curl, CURLOPT_PROGRESSFUNCTION,
                                  &CurlStreamingStrategy::curl_progressfunc );
        curl_easy_setopt( m_curl, CURLOPT_PROGRESSDATA, this );
        m_connected = true; 
        // do the blocking-fetch in a thread:
        m_thread = new boost::thread( boost::bind( &CurlStreamingStrategy::curl_perform, this ) );
    }
    
    CURL *m_curl;
    CURLcode m_curlres;
    std::vector<std::string> m_extra_headers; 
    bool m_connected;
    std::string m_url;
    std::deque< char > m_buffers; // received data
    boost::mutex m_mut;
    boost::condition m_cond;
    bool m_curl_finished;
    size_t m_bytesreceived;
    char m_curlerror[CURL_ERROR_SIZE];
    boost::thread * m_thread;
    bool m_abort;
};

}

#endif
