#include "playdar/application.h"
#include "playdar/resolver_service.h"

bool 
ResolverService::report_results(query_uid qid, 
 vector< ri_ptr > results, string via)
{
    return resolver()->add_results(qid, results, via);
}
