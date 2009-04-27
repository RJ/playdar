#ifndef __RESOLVED_ITEM_H__
#define __RESOLVED_ITEM_H__

#include <string>
#include <boost/function.hpp>
#include "playdar/types.h"
#include "json_spirit/json_spirit.h"

namespace playdar {

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
        j.push_back( Pair("score", (double) m_score) );
        j.push_back( Pair("source", m_source) );
        
        create_json( j );
        return j;
    }
    
    const source_uid& id() const        { return m_uuid; }
    void set_id(const source_uid& s)    { m_uuid = s; }

    const float score() const           { return m_score; }
    const std::string& source() const   { return m_source; }
    
    void set_score(float s)
    { 
        assert(s <= 1.0);
        assert(s >= 0);
        m_score  = s; 
    }
    void set_source(const std::string& s)   { m_source = s; }
    
    virtual void set_url(const std::string& s)      { m_url = s; }
    virtual const std::string & url() const  { return m_url; }
    
    //TODO: move this into PlayableItem somehow
    virtual void set_streaming_strategy(boost::shared_ptr<class StreamingStrategy> s){}
    virtual boost::shared_ptr<class StreamingStrategy> streaming_strategy() const 
    {
        return boost::shared_ptr<class StreamingStrategy>();
    }
    
protected:
    virtual void create_json( json_spirit::Object& ) const = 0;

    
private:
    float m_score;
    std::string m_source;
    std::string m_url;
 
    source_uid m_uuid;
};

}

#endif //__RESOLVED_ITEM_H__
