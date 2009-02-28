#ifndef __RESOLVER__H__
#define __RESOLVER__H__

#include <list>
#include <boost/thread/mutex.hpp>

#include "application/types.h"
#include "resolvers/resolver_query.h"
#include "resolvers/resolver_service.h"
#include "resolvers/playable_item.h"

#include "streaming_strategy.h"
#include "ss_localfile.h"
#include "ss_http.h"
#include "ss_http.h"

class MyApplication;
class ResolverService;
/*
 *  Acts as a container for all content-resolution queries that are running
 *  or waiting to be collected.
 *
 *  To start a query:    ->dispatch(ResolverQuery)
 *  
 *
 */
class Resolver
{
public:
    Resolver(MyApplication * app);
    query_uid dispatch(boost::shared_ptr<ResolverQuery> rq, bool local_only = false);
    MyApplication * app(){ return m_app; }
    bool add_results(query_uid qid,  vector< boost::shared_ptr<PlayableItem> > results, ResolverService * rs);
    vector< boost::shared_ptr<PlayableItem> > get_results(query_uid qid);
    void end_query(query_uid qid);
    int num_results(query_uid qid);
    
    bool query_exists(const query_uid & qid);
    bool add_new_query(boost::shared_ptr<ResolverQuery> rq);

    boost::shared_ptr<ResolverQuery> rq(query_uid qid);
    boost::shared_ptr<PlayableItem> get_pi(source_uid sid);

private:
    query_uid generate_qid();
    source_uid generate_sid();
    
    MyApplication * m_app;
    
    map< query_uid, boost::shared_ptr<ResolverQuery> > m_queries;
    map< source_uid, boost::shared_ptr<PlayableItem> > m_pis;
    
    unsigned int m_id_counter;

    ResolverService * m_rs_local;
    ResolverService * m_rs_lan;
    ResolverService * m_rs_http_playdar;
    ResolverService * m_rs_http_gateway_script;
    ResolverService * m_rs_darknet;
    
    boost::mutex m_mut;
};

#endif

