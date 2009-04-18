#ifndef __RESOLVER_SERVICE_H__
#define __RESOLVER_SERVICE_H__
// Interface for all resolver services

//#include "playdar/application.h"
//#include "playdar/resolver.h"
//#include "playdar/auth.hpp"


#include <DynamicClass.hpp>
#include <iostream> // that HAS to be removed!
#include <string>

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
    // destructor is protected, see below
    
    virtual void Destroy()
    { std::cout << "Unloading " << name() << std::endl; }
    
    /// called once at startup. returning false disables this resolver.
    virtual bool init(pa_ptr pap) = 0;
    
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
    
protected:
    
    virtual ~ResolverService() throw() {  }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// this is what the dynamically loaded resolver plugins extend:
class ResolverServicePlugin 
 : public PDL::DynamicClass,
   public ResolverService
{
public:

    /// Cloning method, this will have to return a new object of the same
    /// derived type
    virtual ResolverService* create( PluginAdaptor* pPA ) = 0;

public:
    DECLARE_DYNAMIC_CLASS( ResolverServicePlugin )

protected:

    template <typename T>
    struct custom_deleter
    {
        void operator() (T *pObj)
        { delete pObj; }
    };

protected:

    template <typename T>
    boost::shared_ptr<T> object_factory()
    { 
        return boost::shared_ptr<T>( new T(), custom_deleter<T>() );
    }

};

////////////////////////////////////////////////////////////////////////////////

}

#endif
