#include "playdar/application.h"
#include "playdar/library.h"
#include "playdar/resolver.h"
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <playdar/playdar_request_handler.h>

using namespace std;

MyApplication::MyApplication(Config c)
    : m_config(c)
{
    string db_path = conf()->get<string>("db", "");
    m_library   = new Library(db_path);
    m_resolver  = new Resolver(this);
}

MyApplication::~MyApplication()
{
    cout << "DTOR MyApplication" << endl;
    cout << "Stopping resolver..." << endl;
    delete(m_resolver);
    cout << "Stopping library..." << endl;
    delete(m_library);
}
     
void
MyApplication::shutdown(int sig)
{

    cout << "Stopping http(" << sig << ")..." << endl;
    if(m_stop_http) m_stop_http();
}
    
Library * 
MyApplication::library()
{ 
    return m_library; 
}

Resolver *
MyApplication::resolver()
{
    return m_resolver;
}






