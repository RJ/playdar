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

    playdar_response authed_http_handler(const playdar_request& req, playdar::auth* pauth);
    playdar_response anon_http_handler(const playdar_request&);

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
