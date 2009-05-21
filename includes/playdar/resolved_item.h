/*
    Playdar - music content resolver
    Copyright (C) 2009  Richard Jones
    Copyright (C) 2009  Last.fm Ltd.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
    
    json_spirit::Object get_json( bool stripUrl = false ) const
    {
        using namespace json_spirit;
        
        Object o;
        map_to_obj( m_jsonmap, o );
        return o;
    }
    
    void rm_json_value( const std::string& v ){ m_jsonmap.erase( v ); }
    const source_uid id() const         { return json_value( "sid", ""); }
    void set_id(const source_uid& s)    { set_json_value( "sid", s ); }

    void set_score( const double s )    { set_json_value( "score", s ); }
    const float score() const           { return (float) json_value( "score",  -1.0); }
    void set_preference( const short p ){ set_json_value( "preference", p ); }
    const short preference() const      { return json_value( "preference",  -1); }
    
    const std::string source() const    { return json_value( "source", "" ); }
    
    virtual void set_url(const std::string& s)  { m_jsonmap["url"] = s; }
    virtual const std::string url() const  { return json_value( "url", "" ); }
    
    /// extra headers to send in the request for this url.
    /// typically only used for http urls, but could be implemented for 
    /// any other protocol.
    /// Expects:  "extra_headers" : [ "Cookie: XXX", "X-Foo: bar" ]
    virtual std::vector<std::string> get_extra_headers() const
    {
        using namespace json_spirit;
        std::vector<std::string> headers;
        std::map< std::string, Value >::const_iterator i = m_jsonmap.find( "extra_headers" );
        if( i != m_jsonmap.end() && i->second.type() == array_type )
        {
            BOOST_FOREACH( Value v, i->second.get_array() )
            {
                if( v.type() != str_type ) continue;
                headers.push_back( v.get_str() );
            }
        }
        return headers;
        
    }
    
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
