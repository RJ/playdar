#ifndef __MYAPPLICATION_H__
#define __MYAPPLICATION_H__
// for windows:
#define NOMINMAX

#include "playdar/playable_item.hpp"
#include "playdar/config.hpp"
#include "playdar/types.h"
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>

#define VERSION "0.1.0"

//:::
//#include "playdar/playdar_request_handler.h"

//namespace moost{ namespace http{ class server; } } // fwd

class Library;
class Resolver;
class ResolverService;
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

    Library * library();
    Resolver * resolver();
    
    Config * conf()
    {
        return &m_config;
    }
    
    // RANDOM UTILITY FUNCTIONS TOSSED IN HERE FOR NOW:
    
    // swap the http://DOMAIN:PORT bit at the start of a URI
    string http_swap_base(const std::string& orig, const std::string& newbase)
    {
        if(orig.at(0) == '/') return newbase + orig;
        size_t slash = orig.find_first_of('/', 8); // https:// is at least 8 in
        if(string::npos == slash) return orig; // WTF?
        return newbase + orig.substr(slash);
    }
    
    // functor that terminates http server:
    void set_http_stopper(boost::function<void()> f) { m_stop_http=f; }
    
    void shutdown(int sig = -1);
    
private:
    
    boost::function<void()> m_stop_http;
    Config m_config;
    Library * m_library;
    Resolver * m_resolver;

};



#endif

