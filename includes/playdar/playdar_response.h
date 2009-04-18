#ifndef __PLAYDAR_RESPONSE_H__
#define __PLAYDAR_RESPONSE_H__

#include <string>

namespace playdar {

class playdar_response {
public:
    playdar_response( const char* s, bool isBody = true ): m_string( s )
    {
        init( s, isBody );
    }

    playdar_response( const std::string& s, bool isBody = true ): m_string( s )
    {
        init( s.c_str(), isBody );
    }
    
    void init( const char* s, bool isBody )
    {
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
        << "<a href=\"/stats/\">Stats</a>"
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
    
    operator const std::string &() const
    { 
        return m_string;
    }
    
    const std::string& str() const{ return m_string; }
    
private:
    std::string m_string;
};

}

#endif //__PLAYDAR_RESPONSE_H__

