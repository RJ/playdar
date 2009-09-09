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

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include "playdar/auth.h"
#include "auth_sql.h"
#include "playdar/logger.h"

namespace playdar {

using namespace std;

auth::auth(const string& dbfilepath)
    : m_db(dbfilepath.c_str())
{
    m_dbfilepath = dbfilepath;
    check_db();
}
    
bool 
auth::is_valid(const string& token, string& whom)
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
    
vector< map<string, string> >
auth::get_all_authed()
{
    boost::mutex::scoped_lock lock(m_mut);
    vector< map<string, string> > ret;
    sqlite3pp::query qry(m_db, "SELECT token, website, name, ua FROM playdar_auth ORDER BY mtime DESC");
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        map<string, string> m;
        m["token"]   = string((*i).get<const char *>(0));
        m["website"] = string((*i).get<const char *>(1));
        m["name"]    = string((*i).get<const char *>(2));
        m["ua"]      = string((*i).get<const char *>(3));
        ret.push_back( m );
    }
    return ret;
}

void
auth::deauth(const string& token)
{
    boost::mutex::scoped_lock lock(m_mut);
    string sql = "DELETE FROM playdar_auth WHERE token = ?";
    sqlite3pp::command cmd(m_db, sql.c_str());
    cmd.bind(1, token.c_str(), true);
    cmd.execute();
}

void 
auth::create_new(const string& token, const string& website, const string& name, const string& ua )
{
    boost::mutex::scoped_lock lock(m_mut);
    string sql = "INSERT INTO playdar_auth "
                 "(token, website, name, ua, mtime, permissions) "
                 "VALUES(?, ?, ?, ?, ?, ?)";
    sqlite3pp::command cmd(m_db, sql.c_str());
    cmd.bind(1, token.c_str(), true);
    cmd.bind(2, website.c_str(), true);
    cmd.bind(3, name.c_str(), true);
    cmd.bind(4, ua.c_str(), true);
    cmd.bind(5, 0);
    cmd.bind(6, "*", true);
    cmd.execute();
}

void 
auth::add_formtoken(const string& ft)
{
    boost::mutex::scoped_lock lock(m_mut);
    m_formtokens.insert( ft );
}

bool 
auth::consume_formtoken(const string& ft)
{
    boost::mutex::scoped_lock lock(m_mut);
    if(m_formtokens.find(ft) == m_formtokens.end())
    { 
        return false;
    }
    m_formtokens.erase(ft);
    return true;
}

void 
auth::check_db()
{
    try
    {
        sqlite3pp::query qry(m_db, "SELECT value FROM settings WHERE key = 'schema_version'");
        sqlite3pp::query::iterator i = qry.begin();
        if( i == qry.end() )
        {
            // unusual - table exists but doesn't contain this row.
            // could have been created wrongly
            log::error() << 
                "Error, settings table missing schema_version key! "
                "Maybe you created the database wrong, or it's corrupt. "
                "Try deleting the auth database" << endl;
            throw; // not caught here.
        }
        string val = (*i).get<string>(0);
        log::info() << "Auth database schema detected as version " << val << endl;
        // check the schema version is what we expect
        // TODO auto-upgrade to newest schema version as needed.
        if( val != "2" )
        {
            throw; // not caught here
        }
        // OK.
    }
    catch(sqlite3pp::database_error err)
    {
        // probably doesn't exist yet, try and create it
        // 
        log::error() << "database_error: " << err.what() << endl;
        create_db_schema();
    }
}

void 
auth::create_db_schema()
{
    log::info() << "Attempting to create DB schema..." << endl;
    string sql( playdar::get_auth_sql() );
    vector<string> statements;
    boost::split( statements, sql, boost::is_any_of(";") );
    BOOST_FOREACH( string s, statements )
    {
        boost::trim( s );
        if (!s.empty()) {
            log::info() << "Executing: " << s << endl;
            sqlite3pp::command cmd(m_db, s.c_str());
            cmd.execute();
        }
    }
    log::info() << "Schema created, reopening." << endl;
    m_db.connect( m_dbfilepath.c_str() ); // this will close/flush and reopen the file
}


} //ns


