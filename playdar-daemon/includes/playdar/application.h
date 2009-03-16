#ifndef __MYAPPLICATION_H__
#define __MYAPPLICATION_H__
// for windows:
#define NOMINMAX

#include <boost/asio.hpp>

#include "playdar/types.h"
#include "playdar/config.hpp"

#include "playdar/playable_item.hpp"
#include <boost/thread.hpp>

#include <boost/program_options.hpp>

//:::
//#include "playdar/playdar_request_handler.h"

using namespace std;

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
class PLAYDAR_DLLEXPORT MyApplication
{
public:
    MyApplication(playdar::Config c);
    ~MyApplication();
    /*
    std::string name();
    unsigned short http_port();
    unsigned short multicast_port();
    boost::asio::ip::address_v4 private_ip();
    boost::asio::ip::address_v4 public_ip();
    boost::asio::ip::address_v4 multicast_ip();
    string httpbase();
    */
    Library * library();
    Resolver * resolver();
    //boost::program_options::variables_map popt(){ return m_po ; }
    
    //template <typename T> T option(string o, T def);
    //template <typename T> T option(string o);
    
    playdar::Config * conf()
    {
        return &m_config;
    }
    
    // RANDOM UTILITY FUNCTIONS TOSSED IN HERE FOR NOW:
    
    // swap the http://DOMAIN:PORT bit at the start of a URI
    string http_swap_base(const string & orig, const string & newbase)
    {
        if(orig.at(0) == '/') return newbase + orig;
        size_t slash = orig.find_first_of('/', 8); // https:// is at least 8 in
        if(string::npos == slash) return orig; // WTF?
        return newbase + orig.substr(slash);
    }
    
    static int levenshtein(const std::string & first, const std::string & second);

    
private:
    
    boost::shared_ptr<boost::asio::io_service::work> m_work;
    boost::shared_ptr<boost::asio::io_service> m_ios;
    
    playdar::Config m_config;
    Library * m_library;
    Resolver * m_resolver;

};



#endif

