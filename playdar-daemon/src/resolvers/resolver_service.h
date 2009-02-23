#ifndef __RESOLVER_SERVICE_H__
#define __RESOLVER_SERVICE_H__
// Interface for all resolver services
#include "resolvers/resolver.h"
#include "application/application.h"

class ResolverService
{
public:
    ResolverService(MyApplication * a)
    {
        m_app = a;
    }
    virtual ~ResolverService(){}

    virtual std::string name() = 0;

    virtual void start_resolving(boost::shared_ptr<ResolverQuery> rq) = 0;

    virtual bool report_results(query_uid qid, vector< boost::shared_ptr<PlayableItem> > results);
    
//protected:
    MyApplication * app() { return m_app; }
private:
    MyApplication * m_app;
};
#endif
