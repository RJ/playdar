#ifndef __RESOLVER__H__
#define __RESOLVER__H__

#include <list>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include "playdar/types.h"
#include "playdar/resolver_query.hpp"
#include "playdar/resolver_service.h"

#include <DynamicClass.hpp>

#include "playdar/streaming_strategy.h"
#include "playdar/ss_localfile.hpp"
#include "playdar/ss_curl.hpp"

namespace playdar {

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
    ~Resolver();
    void detect_curl_capabilities();
    void load_resolver_plugins();
    void load_resolver_scripts();
    query_uid dispatch(boost::shared_ptr<ResolverQuery> rq);
    query_uid dispatch(boost::shared_ptr<ResolverQuery> rq, rq_callback_t cb);
                    
    MyApplication * app(){ return m_app; }
    bool add_results(query_uid qid,  
                     const std::vector< ri_ptr >& results,
                     std::string via);
    std::vector< ri_ptr > get_results(query_uid qid);
    int num_results(query_uid qid);
    
    bool query_exists(const query_uid & qid);
    bool add_new_query(boost::shared_ptr<ResolverQuery> rq);
    void cancel_query(const query_uid & qid);
    void cancel_query_timeout(query_uid qid);

    ri_ptr ri_from_json( const json_spirit::Object& ) const;

    rq_ptr rq(const query_uid & qid);
    ss_ptr get_ss(const source_uid & sid);
    
    size_t num_seen_queries();
    
    const std::vector< pa_ptr >& resolvers() const
    { return m_resolvers; }

    ResolverService * get_resolver(std::string name)
    {
        if( m_pluginNameMap.find( name ) == m_pluginNameMap.end())
            return 0;

        return m_pluginNameMap[ name ];
    }

    void qids(std::deque< query_uid >& out)
    {
        boost::mutex::scoped_lock lock(m_mut_qidlist);
        out = m_qidlist;
    }
    
    /// number of seconds queries should survive for since last being used/accessed.
    /// when this time expires, queries and associated results will be deleted to free memory.
    const time_t max_query_lifetime() const
    {
        return 21600; // 6 hours.
    }
    
    bool pluginadaptor_sorter(const pa_ptr& lhs, const pa_ptr& rhs);
    
    void run_pipeline_cont( rq_ptr rq, 
                       unsigned short lastweight,
                       boost::shared_ptr<boost::asio::deadline_timer> oldtimer);
    void run_pipeline( rq_ptr rq, unsigned short lastweight );
    
    void dispatch_runner();
    
protected:
    float calculate_score( const rq_ptr & rq,  // query
                           const ri_ptr & ri,  // candidate
                           std::string & reason );  // fail reason

private:
    void load_library_resolver();
    
    boost::asio::io_service::work * m_work;
    boost::asio::io_service * m_io_service;
    
    query_uid generate_qid();
    source_uid generate_sid();
    
    MyApplication * m_app;
    
    std::map< query_uid, rq_ptr > m_queries;
    std::map< source_uid, ri_ptr > m_sid2ri;
    // timers used to auto-cancel queries that are inactive for long enough:
    std::map< query_uid, boost::asio::deadline_timer* > m_qidtimers;
    
    // newest-first list of dispatched qids:
    std::deque< query_uid > m_qidlist;
    boost::mutex m_mut_qidlist;
    
    bool m_exiting;
    boost::thread * m_t;
    boost::thread * m_iothr;
    unsigned int m_id_counter;

    // resolver plugin pipeline:
    std::vector< pa_ptr > m_resolvers;

    std::map< std::string, ResolverService* > m_pluginNameMap;
    
    boost::mutex m_mut_results; // when adding results
    
    // for dispatching to the pipeline:
    std::deque< std::pair<rq_ptr, unsigned short> > m_pending;
    boost::mutex m_mutex;
    boost::condition m_cond;

    // StreamingStrategy factories
    std::map< std::string, boost::function<ss_ptr(std::string)> > m_ss_factories;
    
    template <class T>
    boost::shared_ptr<T> ss_ptr_generator(std::string url);
    
};

}

#endif

