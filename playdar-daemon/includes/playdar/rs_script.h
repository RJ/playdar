#ifndef __RS_HTTP_rs_script_H__
#define __RS_HTTP_rs_script_H__

#include <boost/process.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include <vector>
#include <iostream>

#include "playdar/resolver_service.h"

namespace playdar {
namespace resolvers {

namespace bp = ::boost::process;

class rs_script : public ResolverService
{
    public:
    rs_script(){}
    
    void init(playdar::Config * c, Resolver * r, string script);
    
    void start_resolving(rq_ptr rq);
    std::string name() const
    { 
        return m_name; 
    }
    
    /// max time in milliseconds we'd expect to have results in.
    unsigned int target_time() const
    {
        return m_targettime;
    }
    
    /// highest weighted resolverservices are queried first.
    unsigned short weight() const
    {
        return m_weight;
    }
    
    void run();
    
    protected:
        ~rs_script() throw();
        
    private:
    
        int m_weight;
        int m_targettime;
        string m_name;
    
        void init_worker();
        void process_output();
        
        bool m_dead;
        bool m_got_settings;
        string m_scriptpath;
        bp::child * m_c;
        boost::thread * m_t;
        bp::postream * m_os;
        
        bool m_exiting;
        boost::thread * m_dt;
        deque<rq_ptr> m_pending;
        boost::mutex m_mutex;
        boost::condition m_cond;
        
        // used to wait for settings object from script:
        boost::mutex m_mutex_settings;
        boost::condition m_cond_settings;
};



}}
#endif
