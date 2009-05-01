#ifndef __RS_LOCAL_LIBRARY_H__
#define __RS_LOCAL_LIBRARY_H__

#include <deque>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

// All resolver plugins should include this header: 
#include "playdar/playdar_plugin_include.h"

namespace playdar {
    class Library;
namespace resolvers {


class local : public ResolverPlugin<local>
{
public:
    local(){}
    bool init(pa_ptr pap);
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

    playdar_response authed_http_handler(const playdar_request* req, playdar::auth* pauth);
    playdar_response anon_http_handler(const playdar_request*);

protected:

    ~local() throw() 
    {
        m_exiting = true;
        m_cond.notify_all();
        m_t->join();
    };
    
private:
    Library* m_library;
    pa_ptr m_pap;

    bool m_exiting;

    std::deque<rq_ptr> m_pending;

    boost::thread* m_t;
    boost::mutex m_mutex;
    boost::condition m_cond;

    std::vector<scorepair> find_candidates(rq_ptr rq, unsigned int limit = 0);

};

EXPORT_DYNAMIC_CLASS( local )

}}
#endif
