#ifndef __ARTIST_H__
#define __ARTIST_H__

#include <string>
#include "types.h"

class Artist
{
public:
    
    Artist(int id, std::string name)
    : m_id(id), m_name(name)
    {
    }
    
    int         id() const   { return m_id; }
    std::string name() const { return m_name; }
    
private:

    int m_id;
    std::string m_name;
};

#endif
