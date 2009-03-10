#ifndef __RESOLVER_SERVICE_H__
#define __RESOLVER_SERVICE_H__
// Interface for all resolver services
#include "playdar/resolver.h"
#include "playdar/application.h"

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

    virtual void start_resolving(boost::shared_ptr<ResolverQuery> rq) = 0;

    virtual bool report_results(query_uid qid, 
        vector< boost::shared_ptr<PlayableItem> > results,
        string via);
    
//protected:
    
    
    DECLARE_DYNAMIC_CLASS( ResolverService )
    
protected:
    virtual ~ResolverService() throw() {  }
    playdar::Config * m_conf;
    Resolver * m_resolver;
};
#endif
