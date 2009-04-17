#ifndef _PLAYDAR_UTILS_URLENCODING_H_
#define _PLAYDAR_UTILS_URLENCODING_H_

#include <curl/curl.h>

namespace playdar { namespace utils {
    std::string url_encode( const std::string & s )
    {
        char* c = curl_easy_escape( 0, s.c_str(), 0 );
        const std::string& ret = c;
        curl_free( c );
        return ret;
    }

    std::string url_decode( const std::string & s )
    {
        char* c = curl_easy_unescape( 0, s.c_str(), 0, 0 );
        const std::string& ret = c;
        curl_free( c );
        return ret;
    }
}}

#endif //_PLAYDAR_UTILS_URLENCODING_H_
