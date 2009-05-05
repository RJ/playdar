#ifndef __PLAYDAR_RESPONSE_H__
#define __PLAYDAR_RESPONSE_H__

#include <string>
#include <map>
#include <sstream>

namespace playdar {

class playdar_response {
public:
    playdar_response( const char* s, bool isBody = true ): m_string( s ), m_responseCode( 200 )
    {
        init( s, isBody );
    }

    playdar_response( const std::string& s, bool isBody = true ): m_string( s ), m_responseCode( 200 )
    {
        init( s.c_str(), isBody );
    }
    
    void init( const char* s, bool isBody )
    {
        add_header( "Content-Type", "text/html" );

        if( !isBody)
        {
            m_string = s;
            return;
        }
        
        std::ostringstream r;
        r   << "<html><head><title>Playdar</title>"
        << "<style type=\"text/css\">"
        << "td { padding: 5px; }"
        << "</style></head><body>"
        << "<h1>Local Playdar Server</h1>"
        << "<a href=\"/\">Home</a>"
        << "&nbsp; | &nbsp;"
        << "<a href=\"/local/stats\">Stats</a>"
        << "&nbsp; | &nbsp;"
        << "<a href=\"/settings/auth/\">Authentication</a>"
        << "&nbsp; | &nbsp;"
        << "<a href=\"/queries/\">Queries</a>"
        << "&nbsp; | &nbsp;"
        << "<a href=\"/settings/config/\">Configuration</a>"
        
        << "&nbsp; || &nbsp;"
        << "<a href=\"http://www.playdar.org/\" target=\"playdarsite\">Playdar.org</a>"
        
        << "<hr style=\"clear:both;\" />";
        
        r << s;
        r << "\n</body></html>";
        m_string = r.str();
    }

    void add_header( const std::string& k, const std::string& v )
    {
        m_headers[k] = v;
    }

    void set_response_code( const int code ) 
    {
        m_responseCode = code;
    }
    
    operator const std::string &() const
    { 
        return m_string;
    }
    
    const std::string& str() const{ return m_string; }

    int response_code() const{ return m_responseCode; }
    const std::map<std::string,std::string>& headers() const{ return m_headers; }
    
private:
    std::string m_string;
    std::map<std::string,std::string> m_headers;
    int m_responseCode;
};

}

#endif //__PLAYDAR_RESPONSE_H__

