#ifndef __TRACK_RESOLVER_QUERY__
#define __TRACK_RESOLVER_QUERY__

#include "resolver_query.hpp"
class TrackRQBuilder : public ResolverQuery {
public:
    static rq_ptr build( string artist, string album, string track )
    {
        rq_ptr rq = rq_ptr( new ResolverQuery );
        rq->set_param( "artist", artist );
        rq->set_param( "album", album );
        rq->set_param( "track", track );
        return rq;
    }
    
    
    // is this a valid / well formed query?
    static bool valid( rq_ptr rq )
    {
        return rq->param_exists( "artist" ) && rq->param( "artist").length()>0 && 
               rq->param_exists( "track" ) && rq->param( "track" ).length()>0;
    }
    
};

#endif //__TRACK_RESOLVER_QUERY__