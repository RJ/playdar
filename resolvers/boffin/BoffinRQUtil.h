#include "playdar/playdar_plugin_include.h"
#include "playdar/resolver_query.hpp"
#include "playdar/types.h"
#include <string>

namespace BoffinRQUtil
{
    playdar::rq_ptr buildTagCloudRequest( const std::string& rql )
    {
        playdar::rq_ptr rq( new playdar::ResolverQuery );
        rq->set_param( "boffin_tags", rql );
        return rq;
    }
    
    playdar::rq_ptr buildRQLRequest( const std::string& rql )
    {
        playdar::rq_ptr rq( new playdar::ResolverQuery );
        rq->set_param( "boffin_rql", rql );
        return rq;        
    }
}