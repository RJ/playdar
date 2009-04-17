#include "playdar/playdar_plugin_include.h"
#include <string>

namespace BoffinRQUtil
{
    rq_ptr buildTagCloudRequest()
    {
        rq_ptr rq( new ResolverQuery );
        rq->set_param( "boffin_tags", "*" );
        return rq;
    }
    
    rq_ptr buildRQLRequest( const std::string& rql )
    {
        rq_ptr rq( new ResolverQuery );
        rq->set_param( "boffin_rql", rql );
        return rq;        
    }
}