#include "playdar/playdar_request.h"

#include <uriparser/Uri.h>
#include <uriparser/UriBase.h>
#include <uriparser/UriDefsAnsi.h>
#include <uriparser/UriDefsConfig.h>
#include <uriparser/UriDefsUnicode.h>
#include <uriparser/UriIp4.h>

playdar_request::playdar_request( const moost::http::request& req )
{
    cout << "constructing playdar_request( " << req.uri << " ) " << endl;
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
    
    if(parts.size() && parts[0]=="") parts.erase(parts.begin());
    if(parts.size() && *(parts.end() -1)=="") parts.erase(parts.end()-1);
}


/// parse a querystring or form post body into a variables map
int
playdar_request::collect_params(const string & url, map<string,string> & vars)
{
    UriParserStateA state;
    UriQueryListA * queryList;
    UriUriA uri;
    state.uri = &uri;
    int qsnum; // how many pairs in querystring
    // basic uri parsing
    if (uriParseUriA(&state, url.c_str()) != URI_SUCCESS)
    {
        cerr << "FAILED TO PARSE QUERYSTRING" << endl;
        return -1;
    }
    
    if ( uriDissectQueryMallocA(&queryList, &qsnum, uri.query.first, 
                                uri.query.afterLast) != URI_SUCCESS)
    {
        // this means valid but empty
        uriFreeUriMembersA(&uri);
        return 0;
    } 
    else
    {
        UriQueryListA * q = queryList;
        for(int j=0; j<qsnum; j++)
        {
            vars[q->key]= (q->value ? q->value : "");
            if(q->next) q = q->next;
        }
        if(queryList) uriFreeQueryListA(queryList);
        uriFreeUriMembersA(&uri);
        return vars.size();
    }
}

string /* static */
playdar_request::unescape( string s )
{
    // +1's for null terminator
    char * n = (char *) malloc(sizeof(char) * s.length() + 1);
    memcpy(n, s.data(), s.length() + 1);
    uriUnescapeInPlaceA(n);

    string ret(n);
    free(n);
    return ret;
}
