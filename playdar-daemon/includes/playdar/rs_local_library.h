#ifndef __RS_LOCAL_LIBRARY_H__
#define __RS_LOCAL_LIBRARY_H__

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

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
    void start_resolving(rq_ptr rq);
    void run();
    void process(rq_ptr rq);
    
    std::string name() const
    { 
        return string("Local Library on ")
               + conf()->name(); 
    }
    
    /// max time in milliseconds we'd expect to have results in.
    unsigned int target_time() const
    {
        return 25;
    }
    
    /// highest weighted resolverservices are queried first.
    unsigned short weight() const
    {
        return 100;
    }
    
    protected:
        MyApplication * app() { return m_app; }
        MyApplication * m_app;
        ~RS_local_library() throw() {};
    private:
        deque<rq_ptr> m_pending;
        boost::mutex m_mutex;
        boost::condition m_cond;
    
;
};

#endif
