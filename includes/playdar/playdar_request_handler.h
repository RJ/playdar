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
#ifndef __PLAYDAR_REQUEST_HANDLER_H__
#define __PLAYDAR_REQUEST_HANDLER_H__

#include <map>
#include <string>
#include <iostream>

#include <moost/http.hpp>

#include "playdar/application.h"
#include "playdar/auth.hpp"

using namespace std;

namespace playdar {

class playdar_request;

class playdar_request_handler : public moost::http::request_handler_base<playdar_request_handler>
{
public:
    void init(MyApplication * app);
    void handle_request(const moost::http::request& req, moost::http::reply& rep);
    MyApplication * app() { return m_app; }
    
private:

    playdar::auth * m_pauth;    

    std::string sid_to_url(source_uid sid); 
    void handle_json_query(std::string query, const moost::http::request& req, moost::http::reply& rep);
    void handle_rest_api( const playdar_request& req, moost::http::reply& rep, std::string permissions);

    void serve_body(const class playdar_response&, moost::http::reply& rep);
    void serve_static_file(const moost::http::request&, moost::http::reply& rep);
    void serve_track( moost::http::reply& rep, int tid);
    void serve_sid( moost::http::reply& rep, source_uid sid);
    void serve_dynamic( moost::http::reply& rep, 
                        std::string tpl, std::map<std::string,std::string> vars);

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

    std::string handle_queries_root(const playdar_request& req);
    MyApplication * m_app;
   
    typedef std::map< const std::string, boost::function<void( const moost::http::request&, moost::http::reply& )> > HandlerMap;
    HandlerMap m_urlHandlers;
    
    bool m_disableAuth;

};

}

#endif // __PLAYDAR_REQUEST_HANDLER_H__

