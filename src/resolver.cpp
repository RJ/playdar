#include "playdar/application.h"

#include <sstream>
#include <boost/foreach.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "playdar/resolver.h"

#include "playdar/rs_local_library.h"
#include "playdar/rs_script.h"
#include "playdar/library.h"

#include "playdar/utils/levenshtein.h"

// PDL stuff:
#include <DynamicLoader.hpp>
#include <DynamicClass.hpp>
#include <DynamicLibrary.hpp>
#include <DynamicLibraryManager.hpp>
#include <LoaderException.hpp>
#include "./platform.h"
// end PDL stuff


Resolver::Resolver(MyApplication * app)
    :m_app(app), m_exiting(false)
{
    m_id_counter = 0;
    cout << "Resolver starting..." << endl;
    
    m_t = new boost::thread(boost::bind(&Resolver::dispatch_runner, this));
    
    // set up io_service with work so it never ends:
    m_io_service = new boost::asio::io_service();
    m_work = new boost::asio::io_service::work(*m_io_service);
    m_iothr = new boost::thread(boost::bind(
                    &boost::asio::io_service::run,
                    m_io_service));
             
    // Initialize built-in local library resolver:
    load_library_resolver();

    // Load all non built-in resolvers:
    try
    {
        load_resolver_scripts(); // external processes
        load_resolver_plugins(); // DLL plugins
    }
    catch(...)
    {
        cout << "Error loading resolver plugins." << endl;
    }
    typedef std::pair<string,ResolverService*> pairx;
    BOOST_FOREACH( pairx px, m_pluginNameMap )
    {
        cout << px.first << ", " ;
    }
    cout << endl;
    
    // sort the list of resolvers by weight, descending:
    boost::function<bool (const loaded_rs &, const loaded_rs &)> sortfun =
        boost::bind(&Resolver::loaded_rs_sorter, this, _1, _2);
    sort(m_resolvers.begin(), m_resolvers.end(), sortfun);
    cout << endl;
    cout << "Loaded resolvers (" << m_resolvers.size() << ")" << endl;
    BOOST_FOREACH( loaded_rs & lrs, m_resolvers )
    {
        cout << "RESOLVER w:" << lrs.weight << "\tt:" << lrs.targettime 
             << "\t[" << (lrs.script?"script":"plugin") << "]  " 
             << lrs.rs->name() << endl;
    }
    cout << endl;
}

void
Resolver::load_library_resolver()
{
    loaded_rs cr;
    cr.rs = new RS_local_library();
    // local library resolver is special, it gets a handle to app:
    ((RS_local_library *)cr.rs)->set_app(m_app);
    cr.weight = cr.rs->weight();
    cr.targettime = cr.rs->target_time();
    if(cr.rs->init(m_app->conf(), this))
    {
        m_resolvers.push_back( cr );
    }else{
        cerr << "Couldn't load local library resolver. This is bad." 
             << endl;
    }
}


Resolver::~Resolver()
{
    m_exiting = true;
    m_cond.notify_one();
    m_t->join();
    delete(m_work);
    m_io_service->stop();
    m_iothr->join();
}

bool
Resolver::loaded_rs_sorter(const loaded_rs & lhs, const loaded_rs & rhs)
{
    return lhs.weight > rhs.weight;
}

/// spawn resolver scripts:
void 
Resolver::load_resolver_scripts()
{
    cout << "Loading resolver scripts:" << endl;
    typedef map<string,string> st;
    vector< st > scripts = app()->conf()->get_scriptlist();
    BOOST_FOREACH( st & p, scripts )
    {
        cout << "-> Loading: " << p["path"] << endl;
        try
        {
            loaded_rs cr;
            cr.script = true;
            cr.rs = new playdar::resolvers::rs_script();
            bool init = ((playdar::resolvers::rs_script *) cr.rs)
                        ->init(app()->conf(), this, p["path"]);
            if(!init)
            {
                cerr << "-> ERROR couldn't initialize." << endl;
                continue;
            }
            
            if(p.find("targettime")!=p.end())
                cr.targettime = boost::lexical_cast<int>(p["targettime"]);
            else
                cr.targettime = cr.rs->target_time();
                
            if(p.find("weight")!=p.end())
                cr.weight = boost::lexical_cast<int>(p["weight"]);
            else
                cr.weight = cr.rs->weight();
            
            if(cr.weight > 0)
            {
                m_resolvers.push_back( cr );
                cout << "-> OK [w:" << cr.weight 
                     << " t:" << cr.targettime
                     << "] " << cr.rs->name() << endl;
            }
        }
        catch(...)
        {
            cout << "Error initializing script" << endl;
        }
    }
}

/// dynamically load resolver plugins:
void 
Resolver::load_resolver_plugins()
{
    namespace bfs = boost::filesystem;
    bfs::directory_iterator end_itr;
    bfs::path p( m_app->conf()->get(string("plugin_path"), string("plugins")) );
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
        if(m_pluginNameMap.find(boost::to_lower_copy(classname)) != m_pluginNameMap.end())
        {
            cerr << "ERROR: Plugin class '"<< classname << "' already in use. "
                 << "Case-insensitivity applies." << endl;
            continue;
        }
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
                dynamicLoader.GetClassInstance< ResolverServicePlugin >
                    ( pluginfile.c_str(), classname.c_str() );
            if( ! instance->init(app()->conf(), this) )
            {
                cerr << "-> ERROR couldn't initialize." << endl;
                instance->Destroy();
                continue;
            }
            
            m_pluginNameMap[ boost::to_lower_copy(classname) ] = instance;

            loaded_rs cr;
            cr.script = false;
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
    try
    {
        pair<rq_ptr, unsigned short> p;
        while(true)
        {
            {
                boost::mutex::scoped_lock lk(m_mutex);
                if(m_pending.size() == 0) m_cond.wait(lk);
                if(m_exiting) break;
                p = m_pending.back();
                m_pending.pop_back();
            }
            run_pipeline( p.first, p.second );
        }
    }
    catch(...)
    {
        cout << "Error exiting Resolver::dispatch_runner" << endl;
    }
    cout << "Resolver dispatch_runner terminating" << endl;
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
    string reason;
    // add these new results to the ResolverQuery object
    BOOST_FOREACH(boost::shared_ptr<PlayableItem> pip, results)
    {
        // resolver fixes the score using a standard algorithm
        // unless a non-zero score was specified by resolver.
        //if(pip->score() < 0.1)
        //{
            float score = calculate_score( m_queries[qid], pip, reason );
            if(score == 0.0) continue;
            pip->set_score( score );
        //}
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

/// caluclate score 0-1 based on how similar the names are.
/// string similarity algo that combines art,alb,trk from the original
/// query (rq) against a potential match (pi).
/// this is mostly just edit-distance, with some extra checks.
/// TODO albums are ignored atm.
float 
Resolver::calculate_score( const rq_ptr & rq, // query
                           const pi_ptr & pi, // candidate
                           string & reason )  // fail reason
{
    using namespace boost;
    // original names from the query:
    string o_art    = trim_copy(to_lower_copy(rq->artist()));
    string o_trk    = trim_copy(to_lower_copy(rq->track()));
    string o_alb    = trim_copy(to_lower_copy(rq->album()));
    // names from candidate result:
    string art      = trim_copy(to_lower_copy(pi->artist()));
    string trk      = trim_copy(to_lower_copy(pi->track()));
    string alb      = trim_copy(to_lower_copy(pi->album()));
    // short-circuit for exact match
    if(o_art == art && o_trk == trk) return 1.0;
    // the real deal, with edit distances:
    unsigned int trked = playdar::utils::levenshtein( 
                            Library::sortname(trk),
                            Library::sortname(o_trk));
    unsigned int arted = playdar::utils::levenshtein( 
                            Library::sortname(art),
                            Library::sortname(o_art));
    // tolerances:
    float tol_art = 1.5;
    float tol_trk = 1.5;
    //float tol_alb = 1.5; // album rating unsed atm.
    
    // names less than this many chars aren't dismissed based on % edit-dist:
    unsigned int grace_len = 6; 
    
    // if % edit distance is greater than tolerance, fail them outright:
    if( o_art.length() > grace_len &&
        arted > o_art.length()/tol_art )
    {
        reason = "artist name tolerance";
        return 0.0;
    }
    if( o_trk.length() > grace_len &&
        trked > o_trk.length()/tol_trk )
    {
        reason = "track name tolerance";
        return 0.0;
    }
    // if edit distance longer than original name, fail them outright:
    if( arted >= o_art.length() )
    {
        reason = "artist name editdist >= length";
        return 0.0;
    }
    if( trked >= o_trk.length() )
    {
        reason = "track name editdist >= length";
        return 0.0;
    }
    
    // combine the edit distance of artist & track into a final score:
    float artdist_pc = (o_art.length()-arted) / (float) o_art.length();
    float trkdist_pc = (o_trk.length()-trked) / (float) o_trk.length();
    return artdist_pc * trkdist_pc;
}



