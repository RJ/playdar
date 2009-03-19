#ifndef __RS_LOCAL_LIBRARY_H__
#define __RS_LOCAL_LIBRARY_H__

#include "playdar/resolver_service.h"

class RS_local_library : public ResolverService
{
    public:
    RS_local_library(){}
    void init(playdar::Config * c, Resolver * r);
    void set_app(MyApplication * a)
    {
        m_app = a;
    }
    void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    std::string name() const
    { 
        return string("Local Library on ")
               + conf()->name(); 
    }
    protected:
        MyApplication * app() { return m_app; }
        MyApplication * m_app;
        ~RS_local_library() throw() {};
    private:
    
;
};

#endif
