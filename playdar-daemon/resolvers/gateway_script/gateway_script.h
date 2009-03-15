#ifndef __RS_HTTP_GATEWAY_SCRIPT_H__
#define __RS_HTTP_GATEWAY_SCRIPT_H__

#include <vector>
#include <iostream>

#include <boost/process.hpp>

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include "playdar/playdar_plugin_include.h"

namespace playdar {
namespace resolvers {

namespace bp = ::boost::process;

class gateway_script : public ResolverService
{
    public:
    gateway_script(){}
    
    void init(playdar::Config * c, Resolver * r);
    
    void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    std::string name() const
    { 
        return string("Gateway script: ")+m_scriptpath; 
    }
    
    protected:
        ~gateway_script() throw() {}
    private:
        void init_worker();
        void process_output();
        void send_input(string s);
        
        bool m_dead;
        string m_scriptpath;
        bp::child * m_c;
        boost::thread m_t;
        bp::postream * m_os;
};



}}
#endif
