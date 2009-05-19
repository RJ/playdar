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
#ifndef __RS_API_H__
#define __RS_API_H__
// All resolver plugins should include this header:
#include "playdar/playdar_plugin_include.h"

namespace playdar {
namespace resolvers {

class api : public ResolverPlugin<api>
{
public:
    api() {}
    
    bool init( pa_ptr );
    void start_resolving(boost::shared_ptr<ResolverQuery> rq) {} //noop
    unsigned short weight() const { return 0; }
    unsigned int target_time() const { return 0; }
    std::string name() const { return "api"; }

    // all we really do is handle http requests:
    bool anon_http_handler(const playdar_request&, playdar_response&, playdar::auth&);
    bool authed_http_handler(const playdar_request&, playdar_response&, playdar::auth& pauth);
    
protected:
    virtual ~api() throw() {}
    
private:
    pa_ptr m_pap;
};

// This makes the plugin loading possible:
EXPORT_DYNAMIC_CLASS( api )

} // resolvers
} // playdar
#endif
