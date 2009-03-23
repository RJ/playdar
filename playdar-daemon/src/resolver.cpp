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
    
    boost::thread t(boost::bind(&Resolver::dispatch_runner, this));
    
    // set up io_service with work so it never ends:
    m_io_service = boost::shared_ptr<boost::asio::io_service>
                   (new boost::asio::io_service);
    m_work = boost::shared_ptr<boost::asio::io_service::work>
             (new boost::asio::io_service::work(*m_io_service));
    boost::thread iothr(boost::bind(&boost::asio::io_service::run, m_io_service.get()));
             
    // Initialize built-in local library resolver:
    loaded_rs cr;
    cr.rs = new RS_local_library();
    // local library resolver is special, it gets a handle to app:
    ((RS_local_library *)cr.rs)->set_app(m_app);
    cr.weight = cr.rs->weight();
    cr.targettime = cr.rs->target_time();
    cr.rs->init(m_app->conf(), this);
    
    m_resolvers.push_back( cr );

    // Load all non built-in resolvers:
    try
    {
        load_resolvers();
    }
    catch(...)
    {
        cout << "Error loading resolver plugins." << endl;
    }
    
    //TODO at this point, user settings could override weights suggested
    //     by resolverservices by modifying the loaded_rs structs.
    
    // sort the list of resolvers by weight, descending:
    boost::function<bool (const loaded_rs &, const loaded_rs &)> sortfun =
        boost::bind(&Resolver::loaded_rs_sorter, this, _1, _2);
    sort(m_resolvers.begin(), m_resolvers.end(), sortfun);
}

bool
Resolver::loaded_rs_sorter(const loaded_rs & lhs, const loaded_rs & rhs)
{
    return lhs.weight > rhs.weight;
}

/// dynamically load resolver plugins:
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
            //cerr << "Skipping '" << pluginfile 
            //     << "' from plugins directory" << endl;
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
            cout << "Loading resolver: " << pluginfile << endl;
            ResolverService * instance = 
                dynamicLoader.GetClassInstance< ResolverService >
                    ( pluginfile.c_str(), classname.c_str() );
            instance->init(app()->conf(), this);
            // does this plugin handle any URLs?
            vector<string> handlers = instance->get_http_handlers();
            if(handlers.size())
            {
                cout << "-> Registering " << handlers.size() << " HTTP handlers" << endl;
                //typedef pair<string, http_req_cb> pair_t;
                BOOST_FOREACH(string url, handlers)
                {
                    cout << "-> " << url <<  endl;
                    m_http_handlers[url] = instance;
                }
            }
            loaded_rs cr;
            cr.rs = instance;
            string rsopt = "resolvers.";
            confopt += classname;
            cr.weight = app()->conf()->get<int>(rsopt + ".weight", 
                                                instance->weight());
            cr.targettime = app()->conf()->get<int>(rsopt + ".targettime", 
                                                    instance->target_time());
            m_resolvers.push_back( cr );
            cout << "-> OK [w:" << cr.weight << " t:" << cr.targettime << "] " 
                 << instance->name() << endl;
        }
        catch( PDL::LoaderException & ex )
        {
            cerr << "-> Error: " << ex.what() << endl;
        }
    }
    cout << "Num Resolvers Loaded: " << m_resolvers.size() << endl;
}

ResolverService *
Resolver::get_url_handler(string url)
{
    if(m_http_handlers.find(url)==m_http_handlers.end()) return 0;
    return m_http_handlers[url];
}

/// start resolving! (non-blocking)
/// returns a query_uid so you can check status of this query later
query_uid 
Resolver::dispatch(boost::shared_ptr<ResolverQuery> rq)
{
    rq_callback_t null_cb;
    return dispatch(rq, null_cb);
}

query_uid 
Resolver::dispatch(boost::shared_ptr<ResolverQuery> rq,
                    rq_callback_t cb) 
{
    if(!rq->valid())
    {
        // probably malformed, eg no track name specified.
        throw;
    }
    boost::mutex::scoped_lock lk(m_mutex);
    if(!add_new_query(rq))
    {
        // already running
        return rq->id();
    }
    if(cb) rq->register_callback(cb);
    m_pending.push_front( pair<rq_ptr, unsigned short>(rq, 999) );
    m_cond.notify_one();
    
    return rq->id();
}

/// thread that loops forever dispatching stuff in the queue
void
Resolver::dispatch_runner()
{
    pair<rq_ptr, unsigned short> p;
    while(true)
    {
        {
            boost::mutex::scoped_lock lk(m_mutex);
            if(m_pending.size() == 0) m_cond.wait(lk);
            p = m_pending.back();
            m_pending.pop_back();
        }
        run_pipeline( p.first, p.second );
    }
}

/// go thru list of resolversservices and dispatch in order
/// lastweight is the weight of the last resolver we dispatched to.
void
Resolver::run_pipeline( rq_ptr rq, unsigned short lastweight )
{
    unsigned short atweight;
    unsigned int mintime;
    bool started = false;
    BOOST_FOREACH( loaded_rs & lrs, m_resolvers )
    {
        if(lrs.weight >= lastweight) continue;
        if(!started)
        {
            atweight = lrs.weight;
            mintime = lrs.targettime;
            started = true;
            cout << "Pipeline at weight: " << atweight << endl;
        }
        if(lrs.weight != atweight)
        {
            // we've dispatched to everything of weight "atweight"
            // and the shortest targettime at that weight is "mintime"
            // so schedule a callaback after mintime to carry on down the
            // chain and dispatch to the next lowest weighted resolver services.
            cout << "Will continue pipeline after " << mintime << "ms." << endl;
            boost::shared_ptr<boost::asio::deadline_timer> 
                t(new boost::asio::deadline_timer( m_work->get_io_service() ));
            t->expires_from_now(boost::posix_time::milliseconds(mintime));
            // pass the timer pointer to the handler so it doesnt autodestruct:
            t->async_wait(boost::bind(&Resolver::run_pipeline_cont, this,
                                      rq, atweight, t));
            break;
        }
        if(lrs.targettime < mintime) mintime = lrs.targettime;
        // dispatch to this resolver:
        cout << "Pipeline dispatching to " << lrs.rs->name() 
             << " (lastweight: " << lastweight << ")" << endl;
        lrs.rs->start_resolving(rq);
    }
}

void
Resolver::run_pipeline_cont( rq_ptr rq, 
                        unsigned short lastweight,
                        boost::shared_ptr<boost::asio::deadline_timer> oldtimer)
{
    cout << "Pipeline continues.." << endl;
    if(rq->solved())
    {
        cout << "Bailing from pipeline: SOLVED @ lastweight: " << lastweight 
             << endl;
    }
    else
    {
        boost::mutex::scoped_lock lk(m_mutex);
        m_pending.push_front( pair<rq_ptr, unsigned short>(rq, lastweight) );
        m_cond.notify_one();
    }
}

// a resolver will report results here
// false means give up on this query, it's over
// true means carry on as normal
bool
Resolver::add_results(query_uid qid, vector< pi_ptr > results, string via)
{
    if(results.size()==0)
    {
        return true;
    }
    boost::mutex::scoped_lock lock(m_mut_results);
    if(!query_exists(qid)) return false; // query was deleted
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
vector< pi_ptr >
Resolver::get_results(query_uid qid)
{
    vector< pi_ptr > ret;
    boost::mutex::scoped_lock lock(m_mut_results);
    if(!query_exists(qid)) return ret; // query was deleted
    return m_queries[qid]->results();
}

void
Resolver::end_query(query_uid qid)
{
    //boost::mutex::scoped_lock lock(m_mut);
    //m_queries.erase(qid);
}

// check how many results we found for this query id
int 
Resolver::num_results(query_uid qid)
{
    {
        boost::mutex::scoped_lock lock(m_mut_results);
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
    if(query_exists(rq->id())) return false;
    m_queries[rq->id()] = rq;
    m_qidlist.push_front(rq->id());
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
