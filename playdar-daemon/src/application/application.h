#ifndef __MYAPPLICATION_H__
#define __MYAPPLICATION_H__
// for windows:
#define NOMINMAX

#include <boost/asio.hpp>

#include "application/types.h"
#include <boost/thread.hpp>
#include "sqlite3pp.h"
#include <boost/program_options.hpp>


using namespace std;

class Library;
class Resolver;

/*  
 *  Container for all application-level settings,
 *  and various important resources like the library
 *  and resolver
 *
 */
class MyApplication
{
public:
    MyApplication(boost::program_options::variables_map opt);
    ~MyApplication();
    std::string name();
    unsigned short http_port();
    unsigned short multicast_port();
    boost::asio::ip::address_v4 private_ip();
    boost::asio::ip::address_v4 public_ip();
    boost::asio::ip::address_v4 multicast_ip();
    string httpbase();
    Library * library();
    Resolver * resolver();
    boost::program_options::variables_map popt(){ return m_po ; }
    static string gen_uuid();
    
    template <typename T> T option(string o, T def);
    template <typename T> T option(string o);

    template <typename T>
    opt(string cat, string key, T def)
    {
        sqlite3pp::query qry(m_db, string("SELECT value FROM setting WHERE lower(category)=? AND lower(key)=?").c_str() );
        qry.bind(1, cat);
        qry.bind(2, key);
        for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
            return boost::lexical_cast<T>((*i).get<const char *>(0));
            break; // should only be one row
        }
        return def;
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
    sqlite3pp::database m_db;
    sqlite3pp::database * db(){ return &m_db; }
    Library * m_library;
    Resolver * m_resolver;
    boost::program_options::variables_map m_po;
    
};



#endif

