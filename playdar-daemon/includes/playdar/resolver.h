#ifndef __RESOLVER__H__
#define __RESOLVER__H__

#include <list>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

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

// This struct adds weights to the ResolverService so the resolver pipeline
// can run them in the appropriate sequence:
struct loaded_rs
{
    ResolverService * rs; 
    unsigned int targettime; // ms before passing to next resolver
    unsigned short weight;   // highest weight runs first.
};


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
    query_uid dispatch(boost::shared_ptr<ResolverQuery> rq);
    query_uid dispatch(boost::shared_ptr<ResolverQuery> rq, rq_callback_t cb);
                    
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
    
    size_t num_seen_queries();
    
    vector<loaded_rs> * resolvers() { return &m_resolvers; }

    ResolverService * get_url_handler(string url);

    const deque< query_uid > & qids() const
    {
        return m_qidlist;
    }
    
    bool loaded_rs_sorter(const loaded_rs & lhs, const loaded_rs & rhs);
    
    void run_pipeline_cont( rq_ptr rq, 
                       unsigned short lastweight,
                       boost::shared_ptr<boost::asio::deadline_timer> oldtimer);
    void run_pipeline( rq_ptr rq, unsigned short lastweight );
    
    void dispatch_runner();
    
    unsigned int solved_at_weight(unsigned short w)
    {
        boost::mutex::scoped_lock lk(m_mut_stats);
        if(m_solved_at_weight.find(w) != m_solved_at_weight.end())
            return m_solved_at_weight[w];
        else
            return 0;
    }
    
private:
    boost::shared_ptr<boost::asio::io_service::work> m_work;
    boost::shared_ptr<boost::asio::io_service> m_io_service;

    
    // maps URLs to plugins that handle them:
    map<string, ResolverService *> m_http_handlers;
    
    query_uid generate_qid();
    source_uid generate_sid();
    
    MyApplication * m_app;
    
    map< query_uid, boost::shared_ptr<ResolverQuery> > m_queries;
    map< source_uid, boost::shared_ptr<PlayableItem> > m_pis;
    // newest-first list of dispatched qids:
    deque< query_uid > m_qidlist;
    
    unsigned int m_id_counter;

    // resolver plugin pipeline:
    vector<loaded_rs> m_resolvers;
    
    boost::mutex m_mut_results; // when adding results
    boost::mutex m_mut_stats;   // when incrementing stats
    
    // for dispatching to the pipeline:
    deque< pair<rq_ptr, unsigned short> > m_pending;
    boost::mutex m_mutex;
    boost::condition m_cond;
    
    // stats for solved queries:
    map<unsigned short, unsigned int> m_solved_at_weight;
};

#endif

