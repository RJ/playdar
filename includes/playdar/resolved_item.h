#ifndef __RESOLVED_ITEM_H__
#define __RESOLVED_ITEM_H__

// must be first because ossp uuid.h is stubborn and name-conflicts with
// the uuid_t in unistd.h. It gets round this with preprocessor magic. But
// this causes PAIN and HEARTACHE for everyone else in the world, so well done
// to you guys at OSSP. *claps*
#ifdef HAS_OSSP_UUID_H
#include <ossp/uuid.h>
#else
// default source package for ossp-uuid doesn't namespace itself
#include <uuid.h> 
#endif

#include <string>
#include "playdar/types.h"

class ResolvedItem
{
public:
    ResolvedItem():m_score( -1.0f )
    {
        
    }
    
    virtual ~ResolvedItem(){};
    
    json_spirit::Object get_json() const
    {
        using namespace json_spirit;
        Object j;
        j.push_back( Pair("_msgtype", "ri") );
        j.push_back( Pair("score", (double)score()) );
        
        create_json( j );
        return j;
    }

    
    const source_uid & id() const
    {
        if(m_uuid.length()==0) // generate it if not already specified
        {
            m_uuid = playdar::Config::gen_uuid();
        }
        return m_uuid; 
    }
    
    void set_id(std::string s) { m_uuid = s; }
    const float score() const       { return m_score; }
    
    void set_score(float s)
    { 
        assert(s <= 1.0);
        assert(s >= 0);
        m_score  = s; 
    }

protected:
    virtual void create_json( json_spirit::Object& ) const = 0;

    
private:
    float m_score;
    std::string m_source;
    
    mutable source_uid m_uuid;

};

#endif //__RESOLVED_ITEM_H__
