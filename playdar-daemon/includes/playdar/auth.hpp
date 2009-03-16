#ifndef _PLAYDAR_AUTH_HPP_
#define _PLAYDAR_AUTH_HPP_
//#include "playdar/application.h"
#include "sqlite3pp.h"


namespace playdar {
// deals with authcodes etc
class auth
{
public:

    auth(string dbfilepath)
        : m_db(dbfilepath.c_str())
    {
    }
    
    bool is_valid(string token, string & whom)
    {
        boost::mutex::scoped_lock lock(m_mut);
        sqlite3pp::query qry(m_db, "SELECT name FROM playdar_auth WHERE token = ?" );
        qry.bind(1, token.c_str(), true);
        for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
            whom = string((*i).get<const char *>(0));
            return true;
        }
        return false;
    }
    
    vector< map<string,string> > get_all_authed()
    {
        boost::mutex::scoped_lock lock(m_mut);
        vector< map<string,string> > ret;
        sqlite3pp::query qry(m_db, "SELECT token, website, name FROM playdar_auth ORDER BY mtime DESC");
        for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
            map<string,string> m;
            m["token"]   = string((*i).get<const char *>(0));
            m["website"] = string((*i).get<const char *>(1));
            m["name"]    = string((*i).get<const char *>(2));
            ret.push_back( m );
        }
        return ret;
    }
    
    void
    deauth(string token)
    {
        boost::mutex::scoped_lock lock(m_mut);
        string sql = "DELETE FROM playdar_auth WHERE token = ?";
        sqlite3pp::command cmd(m_db, sql.c_str());
        cmd.bind(1, token.c_str(), true);
        cmd.execute();
    }
    
    void create_new(string token, string website, string name)
    {
        boost::mutex::scoped_lock lock(m_mut);
        string sql = "INSERT INTO playdar_auth "
                     "(token, website, name, mtime, permissions) "
                     "VALUES(?, ?, ?, ?, ?)";
        sqlite3pp::command cmd(m_db, sql.c_str());
        cmd.bind(1, token.c_str(), true);
        cmd.bind(2, website.c_str(), true);
        cmd.bind(3, name.c_str(), true);
        cmd.bind(4, 0);
        cmd.bind(5, "*", true);
        cmd.execute();
    }
    
    string gen_formtoken()
    {
        string f = playdar::Config::gen_uuid();
        m_formtokens.insert(f);
        return f;
    }
    
    bool consume_formtoken(string ft)
    {
        if(m_formtokens.find(ft) == m_formtokens.end())
        { 
            return false;
        }
        m_formtokens.erase(ft);
        return true;
    }
    
private:
    set<std::string> m_formtokens;
    
    sqlite3pp::database m_db;
    boost::mutex m_mut;
};
} //ns
#endif
