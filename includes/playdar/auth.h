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
#ifndef _PLAYDAR_AUTH_H_
#define _PLAYDAR_AUTH_H_

#include <map>
#include <string>
#include <vector>
#include <set>
#include "sqlite3pp.h"
#include <boost/thread/mutex.hpp>

namespace playdar {

// deals with authcodes etc
class auth
{
public:
    auth(const std::string& dbfilepath);
    
    bool is_valid(const std::string& token, std::string & whom);
    std::vector< std::map<std::string,std::string> > get_all_authed();
    void deauth(const std::string& token);
    void create_new(const std::string& token, const std::string& website, const std::string& name, const std::string& ua);
    void add_formtoken(const std::string& ft);
    bool consume_formtoken(const std::string& ft);
    
private:
    void check_db();
    void create_db_schema();

    std::set<std::string> m_formtokens;
    sqlite3pp::database m_db;
    boost::mutex m_mut;
};

} //ns

#endif
