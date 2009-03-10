#include "playdar/application.h"

#include <sstream>
#include <boost/foreach.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>

#include "playdar/resolver.h"

#include "playdar/rs_local_library.h"
#include "playdar/library.h"

// PDL stuff:
#include <DynamicLoader.hpp>
#include <DynamicClass.hpp>
#include <DynamicLibrary.hpp>
#include <DynamicLibraryManager.hpp>
#include <LoaderException.hpp>
#include <platform.h>
// end PDL stuff


Resolver::Resolver(MyApplication * app)
    :m_app(app)
{
    m_id_counter = 0;
    cout << "Resolver starting..." << endl;
    // Initialize built-in local library resolver:
    m_rs_local = 0;
    m_rs_local  = new RS_local_library();
    // local library resolver is special, it needs handle to app:
    ((RS_local_library *)m_rs_local)->set_app(m_app);
    // normal init:
    m_rs_local->init(m_app->conf(), this);

    
    // Load all non built-in resolvers:
    load_resolvers();
}

// dynamically load resolver plugins:
void 
Resolver::load_resolvers()
{
    namespace bfs = boost::filesystem;
    bfs::directory_iterator end_itr;
    //bfs::path appdir = bfs::initial_path();
    //bfs::path p(appdir.string() +"/plugins/");
    bfs::path p("plugins");
    cout << "Loading resolver plugins from: " 
         << p.string() << endl;
    for(bfs::directory_iterator itr( p ); itr != end_itr; ++itr)
    {
        if ( bfs::is_directory(itr->status()) ) continue;
        string pluginfile = itr->string();
        if(bfs::extension(pluginfile)!=".resolver")
        {
            cerr << "Skipping '" << pluginfile 
                 << "' from plugins directory" << endl;
            continue;
        }
        string classname = bfs::basename(pluginfile);
        string confopt = "resolvers.";
        confopt += classname;
        confopt += ".enabled";
        if(app()->conf()->get<string>(confopt, "yes") == "no")
        {
            cout << "Skipping '" << classname
                 <<"' - disabled in config file." << endl;
            continue;
        }
        try
        {
            PDL::DynamicLoader & dynamicLoader =
                PDL::DynamicLoader::Instance();
            cout << "-> Trying: " << pluginfile << endl;
            ResolverService * instance = 
                dynamicLoader.GetClassInstance< ResolverService >
                    ( pluginfile.c_str(), classname.c_str() );
            instance->init(app()->conf(), this);
            cout << "-> Loaded: " << instance->name() << endl;
            m_resolvers.push_back(instance);
        }
        catch( PDL::LoaderException & ex )
        {
            cerr << "-> Error: " << ex.what() << endl;
        }
    }
    cout << "Num Resolvers Loaded: " << m_resolvers.size() << endl;
}

// start resolving! (non-blocking)
// returns a query_uid so you can check status of this query later
query_uid 
Resolver::dispatch(boost::shared_ptr<ResolverQuery> rq,
                    bool local_only/* = false */) 
{
    if(!rq->valid())
    {
        throw;
    }
    if(!add_new_query(rq))
    {
        //cout<< "RESOLVER: Not dispatching "<<rq->id()<<" - already running." << endl;
        return rq->id();
    }
    cout << "RESOLVER: dispatch("<< rq->id() <<"): " << rq->str() 
         << "  [mode:"<< rq->mode() <<"]" << endl;
    
    // do the local library (blocking) resolver, because it's quick:
    m_rs_local->start_resolving(rq);

    // if it was solved by the local library, dont bother with the other resolvers!
    if(!local_only && !rq->solved())
    {
        // these calls shouldn't block, plugins do their own threading etc.
        BOOST_FOREACH(ResolverService * rs, m_resolvers)
        {
            if(rs) rs->start_resolving(rq);
        }
    }
    return rq->id();
}

query_uid 
Resolver::dispatch(boost::shared_ptr<ResolverQuery> rq,
                    rq_callback_t cb) 
{
    if(!rq->valid())
    {
        throw;
    }
    if(!add_new_query(rq))
    {
        return rq->id();
    }
    cout << "RESOLVER: dispatch-with-cb("<< rq->id() <<"): " 
         << rq->str() << "  [mode:"<< rq->mode() <<"]" << endl;
         
    rq->register_callback(cb);
    
    // do the local library (blocking) resolver, because it's quick:
    m_rs_local->start_resolving(rq);
    
    if(!rq->solved())
    {
        // these calls shouldn't block!
        BOOST_FOREACH(ResolverService * rs, m_resolvers)
        {
            if(rs) rs->start_resolving(rq);
        }
    }
    return rq->id();
}




void 
Resolver::register_callback(query_uid qid, rq_callback_t cb)
{
    boost::mutex::scoped_lock lock(m_mut);
    if(query_exists(qid)) return;
    m_queries[qid]->register_callback(cb);
}

// a resolver will report results here
// false means give up on this query, it's over
// true means carry on as normal
bool
Resolver::add_results(query_uid qid, vector< boost::shared_ptr<PlayableItem> > results, string via)
{
    if(results.size()==0)
    {
        return true;
    }
    boost::mutex::scoped_lock lock(m_mut);
    if(!query_exists(qid)) return false; // query was deleted
    cout << "RESOLVER add_results(" << qid << ", via: '"
         << via <<"')  "<< results.size()<<" results" 
         << endl;
    // add these new results to the ResolverQuery object
    BOOST_FOREACH(boost::shared_ptr<PlayableItem> pip, results)
    {
        m_queries[qid]->add_result(pip);
        // update map of source id -> playable item
        m_pis[pip->id()] = pip;
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
    return m_queries[qid]->results();
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

size_t
Resolver::num_seen_queries()
{
    return m_queries.size();
}

boost::shared_ptr<PlayableItem> 
Resolver::get_pi(source_uid sid)
{
    return m_pis[sid];
}
