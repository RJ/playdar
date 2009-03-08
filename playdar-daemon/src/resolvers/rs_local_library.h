#ifndef __RS_LOCAL_LIBRARY_H__
#define __RS_LOCAL_LIBRARY_H__

#include "resolvers/resolver_service.h"

class RS_local_library : public ResolverService
{
    public:
    RS_local_library(){}
    void init(MyApplication * a);
    // : ResolverService(a)
    //{
   // }

    void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    std::string name() { return string("Local Library on ")+app()->name(); }
    protected:
        ~RS_local_library() throw() {};
    private:
    
;
};

#endif
