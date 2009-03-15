#ifndef __RESOLVER__H__
#define __RESOLVER__H__

#include <list>
#include <boost/thread/mutex.hpp>

#include "playdar/types.h"
#include "playdar/resolver_query.hpp"
#include "playdar/resolver_service.h"
#include "playdar/playable_item.hpp"

#include <DynamicClass.hpp>

#include "playdar/streaming_strategy.h"
#include "playdar/ss_localfile.hpp"
#include "playdar/ss_http.hpp"


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
    void load_resolvers();
    query_uid dispatch(boost::shared_ptr<ResolverQuery> rq, bool local_only = false);
    query_uid dispatch(boost::shared_ptr<ResolverQuery> rq,
                    rq_callback_t cb);
                    
    MyApplication * app(){ return m_app; }
    bool add_results(query_uid qid,  
                     vector< boost::shared_ptr<PlayableItem> > results,
                     string via);
    vector< boost::shared_ptr<PlayableItem> > get_results(query_uid qid);
    void end_query(query_uid qid);
    int num_results(query_uid qid);
    
    bool query_exists(const query_uid & qid);
    bool add_new_query(boost::shared_ptr<ResolverQuery> rq);

    boost::shared_ptr<ResolverQuery> rq(query_uid qid);
    boost::shared_ptr<PlayableItem> get_pi(source_uid sid);
    
    void register_callback(query_uid qid, rq_callback_t cb);
    // hack-o-matic: for interactive mode in main.cpp.
    ResolverService * get_darknet();
    
    size_t num_seen_queries();
    
    vector<ResolverService *> * resolvers() { return &m_resolvers; }

    ResolverService * get_url_handler(string url);

private:
    
    // maps URLs to plugins that handle them:
    map<string, ResolverService *> m_http_handlers;
    
    query_uid generate_qid();
    source_uid generate_sid();
    
    MyApplication * m_app;
    
    map< query_uid, boost::shared_ptr<ResolverQuery> > m_queries;
    map< source_uid, boost::shared_ptr<PlayableItem> > m_pis;
    
    unsigned int m_id_counter;

    ResolverService * m_rs_local; // local library resolver
    vector<ResolverService *> m_resolvers;
    
    boost::mutex m_mut;
};

#endif

