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
#ifndef __RESOLVER_SERVICE_H__
#define __RESOLVER_SERVICE_H__
// Interface for all resolver services

#include <string>

#include <DynamicClass.hpp>

#include "playdar/pluginadaptor.h"
#include "playdar/playdar_response.h"

#include "json_spirit/json_spirit.h"
#include "playdar/types.h"

namespace playdar {

class playdar_request;
class auth;

class ResolverService
{
public:
    /// Constructor
    ResolverService(){}
    virtual ~ResolverService() {}

    /// called once at startup.
    virtual bool init(pa_ptr pap) = 0;

    /// can return an instance of itself: non-cloneable 
    /// resolvers will return an empty shared ptr
    /// note: ResolverServicePlugin are ALWAYS cloneable!
    virtual boost::shared_ptr<ResolverService> clone()
    { return boost::shared_ptr<ResolverService>(); }

    /// plugin name
    virtual std::string name() const = 0;
    
    /// max time in milliseconds we'd expect to have results in.
    virtual unsigned int target_time() const
    { return 1000; }
    
    /// highest weighted resolverservices are queried first.
    virtual unsigned short weight() const
    { return 100; }

    /// results are sorted by score, then preference (descending)
    /// should be 1-100 indicating likely network reliability of sources
    /// or some other user preference used to adjust ranking.
    virtual unsigned short preference() const
    { return weight(); }
    
    /// start searching for matches for this query
    virtual void start_resolving(boost::shared_ptr<ResolverQuery> rq) = 0;
    
    /// implement cancel if there are any resources you can free when a query is no longer needed.
    /// this is important if you are holding any state, or a copy of the RQ pointer.
    virtual void cancel_query(query_uid qid){ /* no-op */ }

    /// return map of protocol -> SS factory function
    /// used for specific SS implementations by plugins.
    /// TODO the custom alloc/dealloc trick discussed FIXME
    virtual std::map< std::string, boost::function<ss_ptr(std::string)> >
    get_ss_factories()
    {
        // default to empty map - ie, no special SS registered.
        std::map< std::string, boost::function<ss_ptr(std::string)> > facts;
        return facts;
    }

    /// Called when an authenticated http request is made.
    /// @return true if http request is handled. 
    /// @param out should be set to the required http response
    virtual bool authed_http_handler(const playdar_request& req, playdar_response& out, playdar::auth& pauth)
    { 
       return false;
    }
    
    /// Called when a non authenticated http request is made or an
    /// authenticated request is made but not handled by authed_http_handler.
    /// @return true if http request is handled. 
    /// @param out should be set to the required http response
    virtual bool anon_http_handler(const playdar_request&, playdar_response& out, playdar::auth& pauth)
    { 
       return false;
    }

    virtual json_spirit::Object get_capabilities() const{ return json_spirit::Object();}

};

////////////////////////////////////////////////////////////////////////
// The dll plugin interface
// "any problem can be solved with an additional layer of indirection"..
class ResolverServicePlugin
: public PDL::DynamicClass,
  public ResolverService
{
public:
    DECLARE_DYNAMIC_CLASS( ResolverServicePlugin )

protected:
    
    virtual ~ResolverServicePlugin() throw () {}
}; 


}

#endif
