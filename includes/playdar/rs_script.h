/*
    Playdar - music content resolver
    Copyright (C) 2009  Richard Jones
    Copyright (C) 2009  Last.fm Ltd.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __RS_SCRIPT_H__
#define __RS_SCRIPT_H__

#include "playdar/resolver_service.h"

#include <boost/process.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include <vector>
#include <deque>
#include <iostream>

namespace playdar {
namespace resolvers {

namespace bp = ::boost::process;

class rs_script : public ResolverService
{
public:
    rs_script(){}
    
    bool init(pa_ptr pap);
    
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
    
    unsigned short preference() const
    {
        return m_preference;
    }
    
    void run();
    
protected:
    ~rs_script() throw();
        
private:
    
    pa_ptr m_pap;
    
    int m_weight;
    int m_preference;
    int m_targettime;
    std::string m_name;

    void init_worker();
    void process_output();
    void process_stderr();
    
    bool m_dead;
    bool m_got_settings;
    std::string m_scriptpath;
    bp::child * m_c;
    boost::thread * m_t; // std out (main comms)
    boost::thread * m_e; // std error (logging)
    bp::postream * m_os;
    
    bool m_exiting;
    boost::thread * m_dt;
    std::deque<rq_ptr> m_pending;
    boost::mutex m_mutex;
    boost::condition m_cond;
    
    // used to wait for settings object from script:
    boost::mutex m_mutex_settings;
    boost::condition m_cond_settings;
};

}}

#endif // __RS_HTTP_rs_script_H__
