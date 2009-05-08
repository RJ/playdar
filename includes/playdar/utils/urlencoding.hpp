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
#ifndef _PLAYDAR_UTILS_URLENCODING_H_
#define _PLAYDAR_UTILS_URLENCODING_H_

#include <curl/curl.h>
#include <boost/algorithm/string.hpp>

namespace playdar { namespace utils {
    inline std::string url_encode( const std::string & s )
    {
        char* c = curl_easy_escape( 0, s.c_str(), 0 );
        const std::string& ret = c;
        curl_free( c );
        return ret;
    }

    inline std::string url_decode( const std::string & s )
    {
        char* c = curl_easy_unescape( 0, s.c_str(), 0, 0 );
        const std::string& ret = c;
        curl_free( c );
        return boost::replace_all_copy( ret, "+", " " );;
    }
}}

#endif //_PLAYDAR_UTILS_URLENCODING_H_
