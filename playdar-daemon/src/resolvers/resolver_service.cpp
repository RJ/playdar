#include "application/application.h"
#include "resolvers/resolver_service.h"

bool 
ResolverService::report_results(query_uid qid, vector< boost::shared_ptr<PlayableItem> > results)
{
    return app()->resolver()->add_results(qid, results, this);
}
