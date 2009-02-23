#ifndef __ARTIST_H__
#define __ARTIST_H__

using namespace std;

class Artist
{
public:
    
    Artist(int id, string name)
        :m_id(id), m_name(name)
    {
    }
    
    int id() const          { return m_id; }
    string name() const     { return m_name; }
    
    
private:

    int m_id;
    string m_name;

};
#endif
