#ifndef __TRACK_RESOLVER_QUERY__
#define __TRACK_RESOLVER_QUERY__

#include "resolver_query.hpp"
class TrackResolverQuery : public ResolverQuery {
public:
    TrackResolverQuery( string artist, string album, string track )
    {
        m_qryobj_map[ "artist" ] = artist;
        m_qryobj_map[ "album" ] = album;
        m_qryobj_map[ "track" ] = track;
    }
    
    
    // is this a valid / well formed query?
    bool valid() const
    {
        return param_exists( "artist" ) && param( "artist").length()>0 && 
               param_exists( "track" ) && param( "track" ).length()>0;
    }
    
    string str() const
    {
        string s = param( "artist" );
        s+=" - ";
        s+=param("album");
        s+=" - ";
        s+=param("track");
        return s;
    }
    
};

#endif //__TRACK_RESOLVER_QUERY__