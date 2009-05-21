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
#include "playdar/playdar_request.h"
#include "playdar/utils/urlencoding.hpp"
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <moost/http.hpp>

using namespace std;

namespace playdar {

playdar_request::playdar_request( const moost::http::request& req )
{
    // Parse params from querystring:
    if( collect_params( req.uri, m_getvars ) == -1 )
    {
        //TODO: handle bad_request
    }
    
    // Parse params from post body, for form submission
    if( req.content.length() && collect_params( std::string("/?")+req.content, m_postvars ) == -1 )
    {
        //TODO: handle bad_request
        //rep = rep.stock_reply(moost::http::reply::bad_request);
    }
    
    /// parse URL parts
    m_url = req.uri.substr(0, req.uri.find("?"));
    
    collect_parts( m_url, m_parts );
    
    // get rid of cruft from leading/trailing "/" and split:
    if(m_parts.size() && m_parts[0]=="") m_parts.erase(m_parts.begin());
}

void 
playdar_request::collect_parts( const std::string & url, std::vector<std::string>& parts )
{
    const std::string& path = url.substr(0, url.find("?"));
    
    boost::split(parts, path, boost::is_any_of("/"));
    
    BOOST_FOREACH( std::string& part, parts )
    {
        part = playdar::utils::url_decode( part );
    }
    
    if(parts.size() && parts[0]=="") parts.erase(parts.begin());
    if(parts.size() && *(parts.end() -1)=="") parts.erase(parts.end()-1);
}


/// parse a querystring or form post body into a variables map
int
playdar_request::collect_params(const std::string & url, std::map<std::string,std::string> & vars)
{
    size_t pos = url.find( "?" );
    
    if( pos == std::string::npos )
        return 0;
    
    std::string querystring = url.substr( pos + 1, url.length());
    
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep("&");
    tokenizer tokens(querystring, sep);

    std::vector<std::string> paramParts;
    for( tokenizer::iterator tok_iter = tokens.begin();
         tok_iter != tokens.end(); ++tok_iter )
    {
        paramParts.clear();
        boost::split( paramParts, *tok_iter, boost::is_any_of( "=" ));
        if( paramParts.size() != 2 )
            return -1;
        
        vars[ playdar::utils::url_decode( paramParts[0] )] = playdar::utils::url_decode( paramParts[1] );
    }
    return vars.size();
}

}
