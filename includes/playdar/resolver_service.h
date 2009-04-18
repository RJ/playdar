#ifndef __RESOLVER_SERVICE_H__
#define __RESOLVER_SERVICE_H__
// Interface for all resolver services

//#include "playdar/application.h"
//#include "playdar/resolver.h"
//#include "playdar/auth.hpp"

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

    virtual std::string name() const = 0;
    
    /// max time in milliseconds we'd expect to have results in.
    virtual unsigned int target_time() const
    { return 1000; }
    
    /// highest weighted resolverservices are queried first.
    virtual unsigned short weight() const
    { return 100; }

    /// start searching for matches for this query
    virtual void start_resolving(boost::shared_ptr<ResolverQuery> rq) = 0;
    
    /// implement cancel if there are any resources you can free when a query is no longer needed.
    /// this is important if you are holding any state, or a copy of the RQ pointer.
    virtual void cancel_query(query_uid qid){ /* no-op */ }

    /// handler for HTTP reqs we are registered for:
    virtual playdar_response http_handler(const playdar_request*, auth* pauth)
    { return "This plugin has no web interface."; }

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
    
    virtual ~ResolverServicePlugin() {}
}; 


}

#endif
