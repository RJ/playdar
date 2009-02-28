#include "application/application.h"
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/thread/mutex.hpp>

#include "resolvers/resolver.h"

#include "resolvers/rs_local_library.h"
#include "resolvers/rs_lan_udp.h"
#include "resolvers/rs_http_playdar.h"
#include "resolvers/rs_http_gateway_script.h"
#include "resolvers/darknet/rs_darknet.h"
#include "library/library.h"



Resolver::Resolver(MyApplication * app)
{
    m_app = app;
    m_id_counter = 0;
    cout << "Resolver started." << endl;
    
    m_rs_local = 0;
    m_rs_lan = 0;
    m_rs_http_playdar = 0;
    m_rs_http_gateway_script = 0;
    m_rs_darknet = 0;
    
    m_rs_local  = new RS_local_library(app);

    //TODO dynamic loading at runtime using PDL or Boost.extension
    if(app->option<string>("resolver.remote_http.enabled")=="yes")
    {
        try
        {
            string rip = app->option<string>("resolver.remote_http.ip");
            unsigned short rport = (unsigned short) (app->option<int>("resolver.remote_http.port")); 
            boost::asio::ip::address_v4 bip = boost::asio::ip::address_v4::from_string(rip);
            m_rs_http_playdar    = new RS_http_playdar(app, bip, rport);
        }
        catch(exception e)
        {
            cerr << "Failed to load remote_http resolver: " << e.what() << endl;
        }
    }
    
    if(app->option<string>("resolver.lan_udp.enabled")=="yes")
    {
        m_rs_lan    = new RS_lan_udp(app);
    }
    if(app->option<string>("resolver.darknet.enabled")=="yes")
    {
        m_rs_darknet    = new RS_darknet(app);
    }
    /*if(app->option<string>("resolver.gateway_http.enabled")=="yes")
    {
        m_rs_http_gateway_script = new RS_http_gateway_script(app);
    }*/
    
}

// start resolving! (non-blocking)
// returns a query_uid so you can check status of this query later
query_uid 
Resolver::dispatch(boost::shared_ptr<ResolverQuery> rq, bool local_only/* = false */) 
{
    if(!add_new_query(rq))
    {
        cout<< "RESOLVER: Not dispatching "<<rq->id()<<" - already running." << endl;
        return rq->id();;
    }
    cout << "RESOLVER: dispatch("<< rq->id() <<"): " << rq->str() 
         << "  [mode:"<< rq->mode() <<"]" << endl;
    
    // do the local library (blocking) resolver, because it's quick:
    m_rs_local->start_resolving(rq);

    // if it was solved by the local library, dont bother with the other resolvers!
    if(!local_only && !rq->solved())
    {
        // these calls shouldn't block, plugins do their own threading etc.
        if(m_rs_http_playdar)           m_rs_http_playdar->start_resolving(rq);
        if(m_rs_lan)                    m_rs_lan->start_resolving(rq);
        if(m_rs_darknet)                m_rs_darknet->start_resolving(rq);
        if(m_rs_http_gateway_script)    m_rs_http_gateway_script->start_resolving(rq);
    }
    return rq->id();
}

// a resolver will report results here
// false means give up on this query, it's over
// true means carry on as normal
bool
Resolver::add_results(query_uid qid, vector< boost::shared_ptr<PlayableItem> > results, ResolverService * rs)
{
    if(results.size()==0)
    {
        return true;
    }
    boost::mutex::scoped_lock lock(m_mut);
    if(!query_exists(qid)) return false; // query was deleted
    cout << "RESOLVER add_results("<<qid<<", '"<< rs->name() 
         <<"')  "<< results.size()<<" results" << endl;
    // add these new results to the ResolverQuery object
    BOOST_FOREACH(boost::shared_ptr<PlayableItem> pip, results)
    {
        m_queries[qid]->add_result(pip);
    } 
    return true;
}

// gets all the current results for a query
// but leaves query active. (ie, results may change later)
vector< boost::shared_ptr<PlayableItem> >
Resolver::get_results(query_uid qid)
{
    vector< boost::shared_ptr<PlayableItem> > ret;
    boost::mutex::scoped_lock lock(m_mut);
    if(!query_exists(qid)) return ret; // query was deleted
    BOOST_FOREACH(boost::shared_ptr<PlayableItem> pip, m_queries[qid]->results())
    {
        m_pis[pip->id()] = pip;
        ret.push_back(pip);
    }
    return ret; //m_queries[qid]->results();
}

void
Resolver::end_query(query_uid qid)
{
    boost::mutex::scoped_lock lock(m_mut);
    m_queries.erase(qid);
}

// check how many results we found for this query id
int 
Resolver::num_results(query_uid qid)
{
    {
        boost::mutex::scoped_lock lock(m_mut);
        if(query_exists(qid)) 
        {
            return m_queries[qid]->results().size();
        }
    }
    cerr << "Query id '"<< qid <<"' does not exist" << endl;
    return 0;
}

// does this qid still exist?
bool 
Resolver::query_exists(const query_uid & qid)
{
    //boost::mutex::scoped_lock lock(m_mut);
    //map< query_uid, boost::shared_ptr<ResolverQuery> >::const_iterator it = m_queries.find(qid);
    return (m_queries.find(qid) != m_queries.end());
}

// true on success, false if it already exists.
bool 
Resolver::add_new_query(boost::shared_ptr<ResolverQuery> rq)
{
    boost::mutex::scoped_lock lock(m_mut);
    if(query_exists(rq->id())) return false;
    m_queries[rq->id()] = rq;
    return true;
}

boost::shared_ptr<ResolverQuery>
Resolver::rq(query_uid qid)
{
    return m_queries[qid];
}

boost::shared_ptr<PlayableItem> 
Resolver::get_pi(source_uid sid)
{
    return m_pis[sid];
}
