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

    ResolvedItem()
    {
        
    }
    
    ResolvedItem( const json_spirit::Object& jsonobj )
    {
        json_spirit::obj_to_map( jsonobj, m_jsonmap );
    }
    
    virtual ~ResolvedItem(){};
    
    json_spirit::Object get_json() const
    {
        using namespace json_spirit;
        
        Object o;
        map_to_obj( m_jsonmap, o);
        return o;
    }
    
    const source_uid id() const         { return json_value( "sid", ""); }
    void set_id(const source_uid& s)    { set_json_value( "sid", s ); }

    void set_score( const double s )     { set_json_value( "score", s ); }
    const float score() const           { return json_value( "score",  -1.0); }
    const std::string source() const    { return json_value( "source", "" ); }
    
    virtual void set_url(const std::string& s)  { m_jsonmap["url"] = s; }
    virtual const std::string url() const  { return json_value( "url", "" ); }
    
    template< typename T >
    bool has_json_value( const std::string& s ) const
    {
        std::map< std::string, json_spirit::Value >::const_iterator i = 
            m_jsonmap.find( s );
        
        return i != m_jsonmap.end() &&
        i->second.type() == json_type<T>();
    }
    
    std::string json_value( const std::string& s, const char* def ) const
    {
        return json_value( s, std::string( def ));
    }
    
    template< typename T >
    T json_value( const std::string& s, const T& def ) const
    {
        std::map< std::string, json_spirit::Value >::const_iterator i = 
            m_jsonmap.find( s );
        
        return i != m_jsonmap.end() && i->second.type() == json_type<T>() 
            ? i->second.get_value<T>()
            : def;
    }
    
    template< typename T >
    void set_json_value( const std::string& k, const T& v )
    {
        m_jsonmap[k] = v;
    }

    void set_source(const std::string& s)   { m_jsonmap["source"] = s; }

    
private:
    std::map< std::string, json_spirit::Value > m_jsonmap;
    
    template< typename T > 
    static json_spirit::Value_type json_type();
    
};
    
template<>
inline json_spirit::Value_type ResolvedItem::json_type<int>() { return json_spirit::int_type; }

template<>
inline json_spirit::Value_type ResolvedItem::json_type<std::string>() { return json_spirit::str_type; }

template<>
inline json_spirit::Value_type ResolvedItem::json_type<double>() { return json_spirit::real_type; }

template<>
inline json_spirit::Value_type ResolvedItem::json_type<bool>() { return json_spirit::bool_type; }
    
}

#endif //__RESOLVED_ITEM_H__
