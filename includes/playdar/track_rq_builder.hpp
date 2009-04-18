#ifndef __TRACK_RESOLVER_QUERY__
#define __TRACK_RESOLVER_QUERY__

#include "resolver_query.hpp"

namespace playdar {

class TrackRQBuilder {
public:
    static rq_ptr build( const string& artist, const string& album, const string& track )
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
        return rq->param_exists( "artist" ) && 
            rq->param_type( "artist" ) == json_spirit::str_type &&
            rq->param( "artist").get_str().length() && 
            rq->param_exists( "track" ) && 
            rq->param_type( "track" ) == json_spirit::str_type &&
            rq->param( "track" ).get_str().length();
    }
    
};

}

#endif //__TRACK_RESOLVER_QUERY__
