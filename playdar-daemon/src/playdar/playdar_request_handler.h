#ifndef __PLAYDAR_REQUEST_HANDLER_H__
#define __PLAYDAR_REQUEST_HANDLER_H__

#include <vector>
#include <string>
#include <iostream>

#include <moost/http.hpp>

#include <uriparser/Uri.h>
#include <uriparser/UriBase.h>
#include <uriparser/UriDefsAnsi.h>
#include <uriparser/UriDefsConfig.h>
#include <uriparser/UriDefsUnicode.h>
#include <uriparser/UriIp4.h>

#include "library/library.h"
#include "application/application.h"

class playdar_request_handler : public moost::http::request_handler_base<playdar_request_handler>
{
public:
    void init(MyApplication * app);
    void handle_request(const moost::http::request& req, moost::http::reply& rep);
    MyApplication * app() { return m_app; }
    
private:

    // un%encode
    string unescape(string s)
    {
        char * n = (char *) malloc(sizeof(char) * s.length()+1);
        memcpy(n, s.data(), s.length());
        uriUnescapeInPlaceA(n);
        string ret(n);
        delete(n);
        return ret;
    }

    string sid_to_url(source_uid sid) 
    {
        string u = app()->httpbase();
        u += "/sid/" + sid;
        return u;
    }

    void handle_json_query(string query, const moost::http::request& req, moost::http::reply& rep);
    void handle_rest_api(map<string, string> querystring, const moost::http::request& req, moost::http::reply& rep);

    void serve_body(string reply, const moost::http::request& req, moost::http::reply& rep);
    void serve_stats(const moost::http::request& req, moost::http::reply& rep);
    void serve_static_file(const moost::http::request& req, moost::http::reply& rep);
    void serve_track(const moost::http::request& req, moost::http::reply& rep, int tid);
    void serve_sid(const moost::http::request& req, moost::http::reply& rep, source_uid sid);
    
    MyApplication * m_app;
    

};

#endif // __PLAYDAR_REQUEST_HANDLER_H__

