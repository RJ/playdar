#ifndef __PLAYDAR_REQUEST_H__
#define __PLAYDAR_REQUEST_H__

#include <moost/http.hpp>
#include <vector>
#include <map>
#include <string>

class playdar_request {
public:
    playdar_request( const moost::http::request& );
    const string& url() const{ return m_url; }
    bool getvar_exists( const string& s ) const{ return m_getvars.find(s) != m_getvars.end(); }
    bool postvar_exists( const string& s ) const{ return m_postvars.find(s) != m_postvars.end(); }
    unsigned int getvar_count() const{ return m_getvars.size(); }
    unsigned int postvar_count() const{ return m_postvars.size(); }
    const string& getvar( const string& s ) const{ return m_getvars.find(s)->second; }
    const string& postvar( const string& s ) const{ return m_postvars.find(s)->second; }
    const vector<string>& parts() const{ return m_parts; }
    
    // un%encode
    static string unescape(string s);

private:
    
    void collect_parts( const string & url, vector<string>& parts );
    int collect_params(const string & url, map<string,string> & vars);
    
    string m_url;
    vector<string> m_parts;
    map<string, string> m_getvars;
    map<string, string> m_postvars;    
};

#endif //__PLAYDAR_REQUEST_H__
