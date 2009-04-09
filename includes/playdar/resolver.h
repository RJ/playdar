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
    bool script;             // true if external process, false if plugin.
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
    ~Resolver();
    void load_resolver_plugins();
    void load_resolver_scripts();
    query_uid dispatch(boost::shared_ptr<ResolverQuery> rq);
    query_uid dispatch(boost::shared_ptr<ResolverQuery> rq, rq_callback_t cb);
                    
    MyApplication * app(){ return m_app; }
    bool add_results(query_uid qid,  
                     vector< ri_ptr > results,
                     string via);
    vector< ri_ptr > get_results(query_uid qid);
    int num_results(query_uid qid);
    
    bool query_exists(const query_uid & qid);
    bool add_new_query(boost::shared_ptr<ResolverQuery> rq);
    void cancel_query(const query_uid & qid);
    void cancel_query_timeout(query_uid qid);

    rq_ptr rq(const query_uid & qid);
    ri_ptr get_ri(const source_uid & sid);
    
    size_t num_seen_queries();
    
    vector<loaded_rs> * resolvers() { return &m_resolvers; }

    ResolverService * get_resolver(string name)
    {
        if( m_pluginNameMap.find( name ) == m_pluginNameMap.end())
            return 0;

        return m_pluginNameMap[ name ];
    }

    const deque< query_uid > & qids() const
    {
        return m_qidlist;
    }
    
    /// number of seconds queries should survive for since last being used/accessed.
    /// when this time expires, queries and associated results will be deleted to free memory.
    const time_t max_query_lifetime() const
    {
        return 21600; // 6 hours.
    }
    
    bool loaded_rs_sorter(const loaded_rs & lhs, const loaded_rs & rhs);
    
    void run_pipeline_cont( rq_ptr rq, 
                       unsigned short lastweight,
                       boost::shared_ptr<boost::asio::deadline_timer> oldtimer);
    void run_pipeline( rq_ptr rq, unsigned short lastweight );
    
    void dispatch_runner();

protected:
    float calculate_score( const rq_ptr & rq,  // query
                          const pi_ptr & pi,  // candidate
                          string & reason );  // fail reason

private:
    void load_library_resolver();
    
    boost::asio::io_service::work * m_work;
    boost::asio::io_service * m_io_service;
    
    query_uid generate_qid();
    source_uid generate_sid();
    
    MyApplication * m_app;
    
    map< query_uid, rq_ptr > m_queries;
    map< source_uid, ri_ptr > m_ris;
    // timers used to auto-cancel queries that are inactive for long enough:
    map< query_uid, boost::asio::deadline_timer* > m_qidtimers;
    
    // newest-first list of dispatched qids:
    deque< query_uid > m_qidlist;
    
    bool m_exiting;
    boost::thread * m_t;
    boost::thread * m_iothr;
    unsigned int m_id_counter;

    // resolver plugin pipeline:
    vector<loaded_rs> m_resolvers;

    map< string, ResolverService* > m_pluginNameMap;
    
    boost::mutex m_mut_results; // when adding results
    
    // for dispatching to the pipeline:
    deque< pair<rq_ptr, unsigned short> > m_pending;
    boost::mutex m_mutex;
    boost::condition m_cond;

};

#endif

