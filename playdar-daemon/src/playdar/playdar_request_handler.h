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

#include "playdar/library.h"
#include "playdar/application.h"

class playdar_request_handler : public moost::http::request_handler_base<playdar_request_handler>
{
public:
    void init(MyApplication * app);
    void handle_request(const moost::http::request& req, moost::http::reply& rep);
    MyApplication * app() { return m_app; }
    
private:

    set<string> m_formtokens;
    string gen_formtoken();
    
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
        string u = app()->conf()->httpbase();
        u += "/sid/" + sid;
        return u;
    }

    void handle_json_query(string query, const moost::http::request& req, moost::http::reply& rep);
    void handle_rest_api(map<string, string> querystring, const moost::http::request& req, moost::http::reply& rep, string permissions);

    void serve_body(string reply, const moost::http::request& req, moost::http::reply& rep);
    void serve_stats(const moost::http::request& req, moost::http::reply& rep);
    void serve_static_file(const moost::http::request& req, moost::http::reply& rep);
    void serve_track(const moost::http::request& req, moost::http::reply& rep, int tid);
    void serve_sid(const moost::http::request& req, moost::http::reply& rep, source_uid sid);
    void serve_dynamic( const moost::http::request& req, moost::http::reply& rep, 
                                        string tpl, map<string,string> vars);
    MyApplication * m_app;
    

};

// deals with authcodes etc
class PlaydarAuth
{
public:
    PlaydarAuth(sqlite3pp::database * d)
        : m_db(d)
    {}
    
    bool is_valid(string token, string & whom)
    {
        boost::mutex::scoped_lock lock(m_mut);
        sqlite3pp::query qry(*m_db, "SELECT name FROM playdar_auth WHERE token = ?" );
        qry.bind(1, token.c_str(), true);
        for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
            whom = string((*i).get<const char *>(0));
            return true;
        }
        return false;
    }
    
    void create_new(string token, string website, string name)
    {
        boost::mutex::scoped_lock lock(m_mut);
        string sql = "INSERT INTO playdar_auth "
                     "(token, website, name, mtime, permissions) "
                     "VALUES(?, ?, ?, ?, ?)";
        sqlite3pp::command cmd(*m_db, sql.c_str());
        cmd.bind(1, token.c_str(), true);
        cmd.bind(2, website.c_str(), true);
        cmd.bind(3, name.c_str(), true);
        cmd.bind(4, 0);
        cmd.bind(5, "*", true);
        cmd.execute();
    }
    
    
private:
    sqlite3pp::database * m_db;
    boost::mutex m_mut;
};


#endif // __PLAYDAR_REQUEST_HANDLER_H__

