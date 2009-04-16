#ifndef __PLAYDAR_REQUEST_HANDLER_H__
#define __PLAYDAR_REQUEST_HANDLER_H__

#include <vector>
#include <string>
#include <iostream>

#include <moost/http.hpp>

#include "playdar/library.h"
#include "playdar/application.h"
#include "playdar/auth.hpp"
         
class playdar_request;

class playdar_request_handler : public moost::http::request_handler_base<playdar_request_handler>
{
public:
    void init(MyApplication * app);
    void handle_request(const moost::http::request& req, moost::http::reply& rep);
    MyApplication * app() { return m_app; }
    
private:

    playdar::auth * m_pauth;    

    string sid_to_url(source_uid sid); 
    void handle_json_query(string query, const moost::http::request& req, moost::http::reply& rep);
    void handle_rest_api( const playdar_request& req, moost::http::reply& rep, string permissions);

    void serve_body(const class playdar_response&, moost::http::reply& rep);
    void serve_stats(const moost::http::request& req, moost::http::reply& rep);
    void serve_static_file(const moost::http::request&, moost::http::reply& rep);
    void serve_track( moost::http::reply& rep, int tid);
    void serve_sid( moost::http::reply& rep, source_uid sid);
    void serve_dynamic( moost::http::reply& rep, 
                        string tpl, map<string,string> vars);

    void handle_auth1( const playdar_request&, moost::http::reply& );
    void handle_auth2( const playdar_request&, moost::http::reply& );
    void handle_crossdomain( const playdar_request& req, moost::http::reply& rep);
    void handle_root( const playdar_request&, moost::http::reply& );
    void handle_shutdown( const playdar_request&, moost::http::reply& );
    void handle_settings( const playdar_request&, moost::http::reply& );
    void handle_queries( const playdar_request&, moost::http::reply& );
    void handle_serve( const playdar_request&, moost::http::reply& );
    void handle_sid( const playdar_request&, moost::http::reply& );
    void handle_quickplay( const playdar_request&, moost::http::reply& );
    void handle_api( const playdar_request&, moost::http::reply& );
    void handle_pluginurl( const playdar_request&, moost::http::reply& );

    string handle_queries_root(const playdar_request& req);
    MyApplication * m_app;
   
    typedef std::map< const string, boost::function<void( const moost::http::request&, moost::http::reply& )> > HandlerMap;
    HandlerMap m_urlHandlers;
    
    bool m_disableAuth;

};


#endif // __PLAYDAR_REQUEST_HANDLER_H__

