#ifndef __CURL_STRAT_H__
#define __CURL_STRAT_H__
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include <curl/curl.h>

#include "playdar/streaming_strategy.h"
#include "playdar/utils/base64.h"


using namespace boost::asio::ip;
/*
    Can stream from anything cURL can.. kinda.
*/
class CurlStreamingStrategy : public StreamingStrategy
{
public:

    CurlStreamingStrategy(string url)
        : m_curl(0), m_url(url)
    {   
        reset();
    }

    ~CurlStreamingStrategy(){ reset(); }
    
    vector<string> & extra_headers() { return m_extra_headers; }

    int read_bytes(char * buf, size_t size)
    {
        if(!m_connected) do_connect();
        if(!m_connected)
        {
            cout << "ERROR: do_connect failed in httpss." << endl;
            if( m_curl ) curl_easy_cleanup( m_curl );
            reset();
            return 0;
        }
        do{
            boost::mutex::scoped_lock lk(m_mut);
            if( m_buffers.size()==0 && !m_curl_finished )
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

    
    string debug()
    { 
        ostringstream s;
        s<< "CurlStreamingStrategy( " << m_url << " )";
        return s.str();
    }
    
    void reset()
    {
        m_connected = false;
        m_bufoffset = 0;
        m_bytesreceived = 0;
        m_curl_finished = false;
        if( m_curl ) curl_easy_cleanup( m_curl );
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
        inst->m_cond.notify_one();
        return len;
    }
    
    static int curl_progressfunc(void *clientp/*this instance*/, double dltotal, double dlnow, 
                                 double ultotal, double ulnow)
    {
        //cout << "Curl has downloaded: " << dlnow << " : " << dltotal << endl;
        return 0; // non-zero aborts download.
    }
    
    /// run in a thread to do the curl transfer
    void curl_perform()
    {
        cout << "doing curl_perform for '" << m_url << "'" << endl;
        // this blocks until transfer complete / error:
        m_curlres = curl_easy_perform( m_curl );
        m_curl_finished = true;
        cout << "curl_perform done. ret: " << m_curlres << " bytes rcvd: " << m_bytesreceived << endl;
        m_cond.notify_one();
        if(m_curlres != 0) cout << "Curl error: " << m_curlerror << endl;
    }
    
protected:

    void do_connect()
    {
        cout << debug() << endl; 
        reset();
        m_curl = curl_easy_init();
        if(!m_curl)
        {
            cout << "Curl init failed" << endl;
            throw;
        }
        curl_easy_setopt( m_curl, CURLOPT_URL, m_url.c_str() );
        curl_easy_setopt( m_curl, CURLOPT_FOLLOWLOCATION, 1 );
        curl_easy_setopt( m_curl, CURLOPT_MAXREDIRS, 5 );
        curl_easy_setopt( m_curl, CURLOPT_USERAGENT, "Playdar (libcurl)" );
        curl_easy_setopt( m_curl, CURLOPT_WRITEFUNCTION, 
                                  &CurlStreamingStrategy::curl_writefunc );
        curl_easy_setopt( m_curl, CURLOPT_WRITEDATA, this );
        curl_easy_setopt( m_curl, CURLOPT_ERRORBUFFER, (char*)&m_curlerror );
        // set to 0 and uncomment if you want ~1 sec callbacks on progress:
        curl_easy_setopt( m_curl, CURLOPT_NOPROGRESS, 1 );
        //curl_easy_setopt( m_curl, CURLOPT_PROGRESSFUNCTION, 
        //                          &CurlStreamingStrategy::curl_progressfunc );
        //curl_easy_setopt( m_curl, CURLOPT_PROGRESSDATA, this );
        m_connected = true; 
        // do the blocking-fetch in a thread:
        boost::thread t( boost::bind( &CurlStreamingStrategy::curl_perform, this ) );
    }
    
    CURL *m_curl;
    CURLcode m_curlres;
    vector<string> m_extra_headers; 
    bool m_connected;
    string m_url;
    deque< char > m_buffers; // received data
    size_t m_bufoffset;
    boost::mutex m_mut;
    boost::condition m_cond;
    bool m_curl_finished;
    size_t m_bytesreceived;
    char m_curlerror[CURL_ERROR_SIZE];
};

#endif
