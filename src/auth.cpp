#include "playdar/auth.h"
#include "playdar/utils/uuid.h"

namespace playdar {

bool auth::is_valid(std::string token, std::string & whom)
{
    boost::mutex::scoped_lock lock(m_mut);
    sqlite3pp::query qry(m_db, "SELECT name FROM playdar_auth WHERE token = ?" );
    qry.bind(1, token.c_str(), true);
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        whom = std::string((*i).get<const char *>(0));
        return true;
    }
    return false;
}

std::vector< std::map<std::string,std::string> > 
auth::get_all_authed()
{
    boost::mutex::scoped_lock lock(m_mut);
    std::vector< std::map<std::string,std::string> > ret;
    sqlite3pp::query qry(m_db, "SELECT token, website, name FROM playdar_auth ORDER BY mtime DESC");
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        std::map<std::string,std::string> m;
        m["token"]   = std::string((*i).get<const char *>(0));
        m["website"] = std::string((*i).get<const char *>(1));
        m["name"]    = std::string((*i).get<const char *>(2));
        ret.push_back( m );
    }
    return ret;
}

void
auth::deauth(std::string token)
{
    boost::mutex::scoped_lock lock(m_mut);
    std::string sql = "DELETE FROM playdar_auth WHERE token = ?";
    sqlite3pp::command cmd(m_db, sql.c_str());
    cmd.bind(1, token.c_str(), true);
    cmd.execute();
}

void 
auth::create_new(std::string token, std::string website, std::string name)
{
    boost::mutex::scoped_lock lock(m_mut);
    std::string sql = "INSERT INTO playdar_auth "
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

std::string 
auth::gen_formtoken()
{
    std::string f = playdar::utils::gen_uuid();
    m_formtokens.insert(f);
    return f;
}

bool 
auth::consume_formtoken(std::string ft)
{
    if(m_formtokens.find(ft) == m_formtokens.end())
    { 
        return false;
    }
    m_formtokens.erase(ft);
    return true;
}
    
} //ns

