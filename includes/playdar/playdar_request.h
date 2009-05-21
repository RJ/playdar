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
#ifndef __PLAYDAR_REQUEST_H__
#define __PLAYDAR_REQUEST_H__

#include <vector>
#include <map>
#include <string>

// forward declaration
namespace moost { namespace http {
    struct request;
}}

namespace playdar {

class playdar_request {
public:
    playdar_request( const moost::http::request& );
    const std::string& url() const{ return m_url; }
    bool getvar_exists( const std::string& s ) const{ return m_getvars.find(s) != m_getvars.end(); }
    bool postvar_exists( const std::string& s ) const{ return m_postvars.find(s) != m_postvars.end(); }
    unsigned int getvar_count() const{ return m_getvars.size(); }
    unsigned int postvar_count() const{ return m_postvars.size(); }
    const std::string getvar( const std::string& s ) const{ return m_getvars.find(s)->second; }
    const std::string postvar( const std::string& s ) const{ return m_postvars.find(s)->second; }
    const std::vector<std::string>& parts() const{ return m_parts; }
    
private:
    
    void collect_parts( const std::string & url, std::vector<std::string>& parts );
    int collect_params(const std::string & url, std::map<std::string,std::string> & vars);
    
    std::string m_url;
    std::vector<std::string> m_parts;
    std::map<std::string, std::string> m_getvars;
    std::map<std::string, std::string> m_postvars;    
};

}

#endif //__PLAYDAR_REQUEST_H__
