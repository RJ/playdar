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
    bool init(playdar::Config * c, Resolver * r);
    void set_app(MyApplication * a)
    {
        m_app = a;
    }
    void start_resolving(rq_ptr rq);
    void run();
    void process(rq_ptr rq);
    
    std::string name() const
    { 
        return "Local Library";
    }
    
    /// max time in milliseconds we'd expect to have results in.
    unsigned int target_time() const
    {
        return 10;
    }
    
    /// highest weighted resolverservices are queried first.
    unsigned short weight() const
    {
        return 100;
    }
    
    float calculate_score( const rq_ptr & rq,  // query
                           const pi_ptr & pi,  // candidate
                           string & reason );  // fail reason

    
    protected:
        MyApplication * app() { return m_app; }
        MyApplication * m_app;
        ~RS_local_library() throw() 
        {
            m_exiting = true;
            m_cond.notify_all();
            m_t->join();
        };
        
    private:
        bool m_exiting;
        boost::thread * m_t;
        deque<rq_ptr> m_pending;
        boost::mutex m_mutex;
        boost::condition m_cond;
        
        vector<scorepair> find_candidates(rq_ptr rq, unsigned int limit = 0);
    
;
};

#endif
