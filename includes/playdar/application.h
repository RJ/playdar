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
#ifndef __MYAPPLICATION_H__
#define __MYAPPLICATION_H__

//#include <boost/asio.hpp>
//#include <boost/program_options.hpp>
//#include <boost/thread.hpp>

#include <string>
#include <boost/function.hpp>

#include "playdar/config.hpp"
#include "playdar/types.h"

#define VERSION "0.1.0"
//:::
//#include "playdar/playdar_request_handler.h"

//namespace moost{ namespace http{ class server; } } // fwd

namespace playdar {

class Resolver;
class playdar_request_handler;

/*  
 *  Container for all application-level settings,
 *  and various important resources like the library
 *  and resolver
 *
 */
class MyApplication
{
public:
    MyApplication(Config c);
    ~MyApplication();

    Resolver * resolver();
    
    Config * conf()
    {
        return &m_config;
    }
    
    // RANDOM UTILITY FUNCTIONS TOSSED IN HERE FOR NOW:
    
    // swap the http://DOMAIN:PORT bit at the start of a URI
    std::string http_swap_base(const std::string& orig, const std::string& newbase)
    {
        if(orig.at(0) == '/') return newbase + orig;
        size_t slash = orig.find_first_of('/', 8); // https:// is at least 8 in
        if(std::string::npos == slash) return orig; // WTF?
        return newbase + orig.substr(slash);
    }
    
    // functor that terminates http server:
    void set_http_stopper(boost::function<void()> f) { m_stop_http=f; }
    
    void shutdown(int sig = -1);
    
private:
    
    boost::function<void()> m_stop_http;
    Config m_config;
    Resolver * m_resolver;

};

}

#endif

