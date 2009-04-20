#ifndef __RS_LOCAL_LIBRARY_H__
#define __RS_LOCAL_LIBRARY_H__

#include <deque>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include "playdar/resolver_service.h"
#include "playdar/pluginadaptor.h"
#include "playdar/types.h"
#include "playdar/application.h"

namespace playdar { namespace resolvers {

class RS_local_library : public ResolverService
{
public:
    RS_local_library(){}

    bool init(pa_ptr pap);
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

    pa_ptr m_pap;

    bool m_exiting;

    std::deque<rq_ptr> m_pending;

    boost::thread* m_t;
    boost::mutex m_mutex;
    boost::condition m_cond;

    std::vector<scorepair> find_candidates(rq_ptr rq, unsigned int limit = 0);

};

}}

#endif
