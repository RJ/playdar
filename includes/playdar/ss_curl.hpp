/*
    Playdar - music content resolver
    Copyright (C) 2009  Richard Jones
    Copyright (C) 2009  Last.fm Ltd.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __CURL_STRAT_H__
#define __CURL_STRAT_H__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <curl/curl.h>

#include "playdar/streaming_strategy.h"

namespace playdar {

using namespace boost::asio::ip;
/*
    Can stream from anything cURL can.. 
    
    TODO handle failure/slow streams better.
    TODO potential efficiency gain by storing data in lumps of size returned by curl
    TODO make some of the curl options user-configurable (ssl cert checking, auth, timeouts..)
*/
class CurlStreamingStrategy 
    : public StreamingStrategy
    , public boost::enable_shared_from_this<CurlStreamingStrategy>
{
public:
    CurlStreamingStrategy(std::string url)
        : m_curl(0)
        , m_slist_headers(0)
        , m_url(url)
        , m_thread(0)
        , m_abort(false)
        , m_contentlen( -1 )
    {
        url = boost::to_lower_copy( m_url );
        std::vector<std::string> parts;
        boost::split( parts, url, boost::is_any_of( ":" ));
        if( parts.size() > 1 )
            m_protocol = parts[0];
        
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

#ifndef NDEBUG
    ~CurlStreamingStrategy()
    {
        std::cout << "DTOR ss_curl: " << m_url << std::endl;
    }
#endif

    /// this returns a shr_ptr to a copy, because it's not threadsafe 
    virtual boost::shared_ptr<StreamingStrategy> get_instance()
    {
        // make a copy:
        return boost::shared_ptr<StreamingStrategy>(new CurlStreamingStrategy(*this));
    }
    
    void set_extra_header(const std::string& header)
    {
        m_slist_headers = curl_slist_append(m_slist_headers, header.c_str());
    }

    std::string mime_type()
    {
        if( m_mimetype.size() )
            return m_mimetype;
        
        if( m_url.size() < 3 )
            return ext2mime( "" );
        
        std::string ext = m_url.substr( m_url.size() - 3 );
        return ext2mime( ext );
    }
    
    std::string debug()
    { 
        std::ostringstream s;
        s << "CurlStreamingStrategy( " << m_url << " )";
        return s.str();
    }
    
    void reset()
    {
        m_bytesreceived = 0;
        m_abort = false;
    }

    /// curl callback when data from fetching an url has arrived
    static size_t curl_headfunc( void *vptr, size_t size, size_t nmemb, void *custom )
    {
        CurlStreamingStrategy * inst = ((CurlStreamingStrategy*)custom);
        char * ptr = (char*) vptr;
        size_t len = size * nmemb;
        std::string s( ptr, len );
        boost::to_lower( s );
        std::vector<std::string> v;
        boost::split( v, s, boost::is_any_of( ":" ));
        if( v.size() != 2 )
            return len;
        
        boost::trim( v[0] );
        boost::trim( v[1] );
        if( v[0] == "content-length" )
            try {
                inst->onContentLength( boost::lexical_cast<int>( v[1] ) );
            }catch( ... )
            {}
        else if( v[0] == "content-type" )
            inst->m_mimetype = v[1];
        
        return len;
    }
    
    /// curl callback when data from fetching an url has arrived
    static size_t curl_writefunc( void *vptr, size_t size, size_t nmemb, void *custom )
    {
        CurlStreamingStrategy * inst = ((CurlStreamingStrategy*)custom);
        size_t len = size * nmemb;
        inst->m_reply->write_content(std::string((char*) vptr, len));
        return len;
    }
    
    static int curl_progressfunc(void *clientp/*this instance*/, double dltotal, double dlnow, 
                                 double ultotal, double ulnow)
    {
        CurlStreamingStrategy * inst = ((CurlStreamingStrategy*)clientp);
        //cout << "Curl has downloaded: " << dlnow << " : " << dltotal << endl;
        if( inst->m_abort )
        {
            std::cout << "Aborting in-progress download." << std::endl;
            return 1; // non-zero aborts download.
        }
        inst->onContentLength(dltotal);
        return 0;
    }
    
    /// run in a thread to do the curl transfer
    void curl_perform()
    {
        std::cout << "doing curl_perform for '" << m_url << "'" << std::endl;
        // this blocks until transfer complete / error:
        m_curlres = curl_easy_perform( m_curl );
        if (m_slist_headers) 
            curl_slist_free_all(m_slist_headers);
        if (m_curlres) {
            std::cout << "Curl error: " << m_curlerror << std::endl;
            m_reply->set_status(500);
            m_reply->write_release();
        }
        m_reply->write_finish();
        curl_easy_cleanup( m_curl );
    }
    
    const std::string url() const 
    { 
        return m_url; 
    }

	void start_reply(moost::http::reply_ptr reply)
	{
        std::cout << debug() << std::endl; 
        reset();
        m_curl = curl_easy_init();
        if(!m_curl)
        {
            std::cout << "Curl init failed" << std::endl;
            throw;
        }
       
        prep_curl( m_curl );
        
		m_reply = reply;
        m_reply->write_hold();              // release once we have a content-length
        m_reply->set_write_ending_cb(
            boost::bind(&CurlStreamingStrategy::write_ending, shared_from_this()));

        // do the blocking-fetch in a thread:
        m_thread = new boost::thread( 
            boost::bind(&CurlStreamingStrategy::curl_perform, shared_from_this()));
    }

protected:

    void onContentLength(int contentLength)
    {
        if (m_contentlen == -1 && contentLength > 0) {
            m_contentlen = contentLength;
            std::cout << "got content-length " << m_contentlen << std::endl;
            m_reply->add_header("Content-Length", m_contentlen);
            m_reply->write_release();
        }
    }
    
    // callback from m_reply: the connection has finished writing.
    void write_ending()
    {
        // release our shared ptrs
        m_reply->set_write_ending_cb(0);
        if (m_thread) {
            m_abort = true;
            m_thread->join();
            //delete m_thread;
        }
    }

    void prep_curl(CURL * handle)
    {
        // for curl options, see:
        // http://curl.netmirror.org/libcurl/c/curl_easy_setopt.html
        curl_easy_setopt( handle, CURLOPT_NOSIGNAL, 1 );
        curl_easy_setopt( handle, CURLOPT_NOBODY, 0 );
        curl_easy_setopt( handle, CURLOPT_CONNECTTIMEOUT, 5 );
        curl_easy_setopt( handle, CURLOPT_SSL_VERIFYPEER, 0 );
        curl_easy_setopt( handle, CURLOPT_FTP_RESPONSE_TIMEOUT, 10 );
        curl_easy_setopt( handle, CURLOPT_URL, m_url.c_str() );
        curl_easy_setopt( handle, CURLOPT_FOLLOWLOCATION, 1 );
        curl_easy_setopt( handle, CURLOPT_MAXREDIRS, 5 );
        curl_easy_setopt( handle, CURLOPT_USERAGENT, "Playdar (libcurl)" );
        curl_easy_setopt( handle, CURLOPT_HTTPHEADER, m_slist_headers );
        curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, &CurlStreamingStrategy::curl_writefunc );
        curl_easy_setopt( handle, CURLOPT_WRITEDATA, this );
        curl_easy_setopt( handle, CURLOPT_HEADERFUNCTION, &CurlStreamingStrategy::curl_headfunc );
        curl_easy_setopt( handle, CURLOPT_HEADERDATA, this );
        curl_easy_setopt( handle, CURLOPT_ERRORBUFFER, (char*)&m_curlerror );
        // we use the curl progress callbacks to abort transfers mid-download on exit
        curl_easy_setopt( handle, CURLOPT_NOPROGRESS, 0 );
        curl_easy_setopt( handle, CURLOPT_PROGRESSFUNCTION,
                         &CurlStreamingStrategy::curl_progressfunc );
        curl_easy_setopt( handle, CURLOPT_PROGRESSDATA, this );
    }
    
    //FIXME: DUPLICATED from scanner.cpp
    //this should be moved into utils
    std::string ext2mime(std::string ext)
    { 
        if(ext=="mp3") return "audio/mpeg";
        if(ext=="aac") return "audio/mp4";
        if(ext=="mp4") return "audio/mp4";
        if(ext=="m4a") return "audio/mp4"; 
        std::cerr << "Warning, unhandled file extension. Don't know mimetype for " << ext << std::endl;
        //generic:
        return "application/octet-stream";
    }

    CURL *m_curl;
    struct curl_slist * m_slist_headers; // extra headers to be sent
    CURLcode m_curlres;
    std::string m_url;
    std::string m_protocol;
    size_t m_bytesreceived;
    char m_curlerror[CURL_ERROR_SIZE];
    boost::thread* m_thread;
    bool m_abort;
    
    int m_contentlen;
    std::string m_mimetype;

    /////
    moost::http::reply_ptr m_reply;
};

}

#endif
