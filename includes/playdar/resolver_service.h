#ifndef __RESOLVER_SERVICE_H__
#define __RESOLVER_SERVICE_H__
// Interface for all resolver services
#include "playdar/application.h"
#include "playdar/resolver.h"
#include "playdar/auth.hpp"
#include "playdar/playdar_response.h"

#include <DynamicClass.hpp>

class playdar_request;
class ResolverService
{
public:
    ResolverService(){}
    
    virtual void Destroy()
    {
        cout << "Unloading " << name() << endl;
    }
    
    /// called once at startup. returning false disables this resolver.
    virtual bool init(pa_ptr pap) = 0;
    
    virtual std::string name() const = 0;
    
    /// max time in milliseconds we'd expect to have results in.
    virtual unsigned int target_time() const
    {
        return 1000;
    }
    
    /// highest weighted resolverservices are queried first.
    virtual unsigned short weight() const
    {
        return 100;
    }

    /// start searching for matches for this query
    virtual void start_resolving(boost::shared_ptr<ResolverQuery> rq) = 0;
    
    /// implement cancel if there are any resources you can free when a query is no longer needed.
    /// this is important if you are holding any state, or a copy of the RQ pointer.
    virtual void cancel_query(query_uid qid){ /* no-op */ }

    /// handler for HTTP reqs we are registered for:
    virtual playdar_response http_handler( const playdar_request&, playdar::auth * pauth)
    {
        return "This plugin has no web interface.";
    }
    
protected:
    
    virtual ~ResolverService() throw() {  }

};

/// this is what the dynamically loaded resolver plugins extend:
class ResolverServicePlugin 
 : public PDL::DynamicClass,
   public ResolverService
{
public:
    DECLARE_DYNAMIC_CLASS( ResolverServicePlugin )
};


#endif
