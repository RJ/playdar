#ifndef __TAG_CLOUD_RQ_BUILDER_H__
#define __TAG_CLOUD_RQ_BUILDER_H__

#include "playdar/resolver_query.hpp"

namespace TagCloudRQBuilder {

    rq_ptr build()
    {
        rq_ptr rq = rq_ptr( new ResolverQuery );
        rq->set_param( "boffin_tags", "*" );
        return rq;
    }
}

#endif //__TAG_CLOUD_RQ_BUILDER_H__