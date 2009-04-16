#include "playdar/playdar_request.h"
#include "playdar/utils/urlencoding.hpp"
#include <boost/tokenizer.hpp>

playdar_request::playdar_request( const moost::http::request& req )
{
    // Parse params from querystring:
    if( collect_params( req.uri, m_getvars ) == -1 )
    {
        //TODO: handle bad_request
    }
    
    // Parse params from post body, for form submission
    if( req.content.length() && collect_params( string("/?")+req.content, m_postvars ) == -1 )
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
playdar_request::collect_parts( const string & url, vector<string>& parts )
{
    const string& path = url.substr(0, url.find("?"));
    
    boost::split(parts, path, boost::is_any_of("/"));
    
    BOOST_FOREACH( string& part, parts )
    {
        part = playdar::utils::url_decode( part );
    }
    
    if(parts.size() && parts[0]=="") parts.erase(parts.begin());
    if(parts.size() && *(parts.end() -1)=="") parts.erase(parts.end()-1);
}


/// parse a querystring or form post body into a variables map
int
playdar_request::collect_params(const string & url, map<string,string> & vars)
{
    size_t pos = url.find( "?" );
    
    if( pos == string::npos )
        return 0;
    
    string querystring = url.substr( pos + 1, url.length());
    
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep("&");
    tokenizer tokens(querystring, sep);

    vector<string> paramParts;
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
