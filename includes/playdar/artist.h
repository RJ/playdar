#ifndef __ARTIST_H__
#define __ARTIST_H__

#include <string>

class Artist
{
public:
    
    Artist(int id, const std::string& name)
        :m_id(id), m_name(name)
    {
    }
    
    int id() const { return m_id; }
    const std::string& name() const { return m_name; }
    
    
private:

    int m_id;
    std::string m_name;

};
#endif
