#ifndef __RESOLVER_SERVICE_H__
#define __RESOLVER_SERVICE_H__
// Interface for all resolver services
#include "playdar/resolver.h"
#include "playdar/application.h"
#include "playdar/auth.hpp"
#include <DynamicClass.hpp>


class ResolverService : public PDL::DynamicClass, std::exception


{
public:
    ResolverService(){}
    
    
    virtual void init(playdar::Config * c, Resolver * r)
    {
        m_resolver = r;
        m_conf = c;
    }
    
    virtual const playdar::Config * conf() const
    {
        return m_conf;
    }
    
    virtual Resolver * resolver() const
    {
        return m_resolver;
    }

    virtual std::string name() const
    {
        return "UNKNOWN_RESOLVER";
    }
    
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

    virtual void start_resolving(boost::shared_ptr<ResolverQuery> rq) = 0;

    virtual bool report_results(query_uid qid, 
        vector< boost::shared_ptr<PlayableItem> > results,
        string via);
    
    // default is empty, ie no http urls handle
    virtual vector<string> get_http_handlers()
    {
        vector<string> h;
        return h;
    }
    
    // handler for HTTP reqs we are registerd for:
    virtual string http_handler( string url,
                                 vector<string> parts,
                                 map<string,string> getvars,
                                 map<string,string> postvars,
                                playdar::auth * pauth)
    {
        return "This plugin has no web interface.";
    }
    
//protected:
    
    
    DECLARE_DYNAMIC_CLASS( ResolverService )
    
protected:
    virtual ~ResolverService() throw() {  }
    playdar::Config * m_conf;
    Resolver * m_resolver;
};
#endif
