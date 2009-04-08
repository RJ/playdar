#include "playdar/application.h"

#include <sstream>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "playdar/resolver.h"

#include "playdar/rs_local_library.h"
#include "playdar/rs_script.h"
#include "playdar/library.h"

// Generic track calculation stuff:
#include "playdar/track_rq_builder.hpp"
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

// work-around lack of basic_path::stem()
//
// Returns: if p.filename() contains a dot, returns the substring of p.filename() 
// starting at its beginning and ending at the last dot (the dot is not included). 
// Otherwise, returns p.filename().
template<typename Path>
typename Path::string_type 
stem(const Path & p)
{
    int pos = p.leaf().find_last_of('.');
    if (pos != Path::string_type::npos) {
        return p.leaf().substr(0, pos);
    } 
    return p.leaf();
}


/// spawn resolver scripts:
void 
Resolver::load_resolver_scripts()
{
    using namespace boost::filesystem;
    
    path const etc = "etc"; //FIXME don't depend on working directory
    cout << "Loading resolver scripts from: " << etc << endl;
    
    directory_iterator const end;
    string name;
    for(directory_iterator i(etc); i != end; ++i) {
        try {
            // basic_path::leaf() understandably renamed to filename() in boost 1.36
            // (leaf method marked as deprecated in boost > 1.35)
            // basic_path::stem() added in boost 1.36
            string name = i->path().leaf();
            string conf = "plugins." + stem( i->path() ) + '.';     
            
            if (is_directory(i->status()) || is_other(i->status()))
                continue;
            //FIXME more sensible place to put resolving scripts
            if (name=="playdar.conf" || name=="schema.sql" || name=="mock-input.pl")
                continue;
            if (app()->conf()->get<bool>(conf+"enabled", true) == false){
                cout << "-> Skipping '"+name+"' - disabled in config file";
                continue;
            }

            cout << "-> Loading: " << name << endl;
            
            loaded_rs cr;
            cr.script = true;
            cr.rs = new playdar::resolvers::rs_script();
            bool init = ((playdar::resolvers::rs_script*) cr.rs)->init(app()->conf(), this, i->path().string());
            if(!init) throw 1;

            cr.targettime = app()->conf()->get<int>(conf+"targettime", cr.rs->target_time());            
            cr.weight = app()->conf()->get<int>(conf+"weight", cr.rs->weight());
            
            if(cr.weight > 0) {
                m_resolvers.push_back( cr );
                cout << "-> OK [weight:" << cr.weight 
                     <<  " target-time:" << cr.targettime
                     << "] " << cr.rs->name() << endl;
             }
        }
        catch(...)
        {
            cerr << "-> ERROR initializing script at: " << *i << endl;
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
        if(app()->conf()->get<bool>(confopt, true) == false)
        {
            cout << "Skipping '" << classname
                 <<"' - disabled in config file" << endl;
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
            cout << "Added pluginName " << boost::to_lower_copy(classname) << endl;
            loaded_rs cr;
            cr.script = false;
            cr.rs = instance;
            string rsopt = "resolvers."+classname;
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
    return dispatch(rq, 0);
}

query_uid 
Resolver::dispatch(boost::shared_ptr<ResolverQuery> rq,
                    rq_callback_t cb) 
{
    assert( rq->valid() );
    boost::mutex::scoped_lock lk(m_mutex);
    if(!add_new_query(rq))
    {
        // already running
        return rq->id();
    }
    if(cb) rq->register_callback(cb);
    m_pending.push_front( pair<rq_ptr, unsigned short>(rq, 999) );
    m_cond.notify_one();
    // set up timer to auto-cancel this query after a while:
    boost::asio::deadline_timer * t = new boost::asio::deadline_timer(*m_io_service);
    // give 5 mins additional time to allow setup/results, otherwise it would never be stale
    // at max_query_lifetime, because the first result updates the atime:
    t->expires_from_now(boost::posix_time::seconds(max_query_lifetime()+300));
    t->async_wait(boost::bind(&Resolver::cancel_query_timeout, this, rq->id()));
    m_qidtimers[rq->id()] = t;
    return rq->id();
}

/// thread that loops forever dispatching stuff in the queue
void
Resolver::dispatch_runner()
{
    try
    {
        while(true)
        {
            pair<rq_ptr, unsigned short> p;
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

/// a resolver will report results here
/// false means give up on this query, it's over
/// true means carry on as normal
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
        rq_ptr rq = m_queries[qid];
        // resolver fixes the score using a standard algorithm
        // unless a non-zero score was specified by resolver.
        if(pip->score() < 0 &&
           TrackRQBuilder::valid( rq ))
        {
            float score = calculate_score( rq, pip, reason );
            if( score == 0.0) continue;
            pip->set_score( score );
        }
        
        m_queries[qid]->add_result(pip);
        // update map of source id -> playable item
        m_pis[pip->id()] = pip;
    } 
    return true;
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
    string o_art    = trim_copy(to_lower_copy(rq->param( "artist" ).get_str()));
    string o_trk    = trim_copy(to_lower_copy(rq->param( "track" ).get_str()));
    string o_alb    = trim_copy(to_lower_copy(rq->param( "album" ).get_str()));

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


/// Cancel a query, delete any results and free the memory. QID will no longer exist.
void 
Resolver::cancel_query(const query_uid & qid)
{
    cout << "Cancelling query: " << qid << endl;
    // send cancel to all resolvers, in case they have cleanup to do:
    BOOST_FOREACH( loaded_rs & lrs, m_resolvers )
    {
        lrs.rs->cancel_query( qid );
    }
    rq_ptr cq;
    {
        // removing from m_queries map means no-one can find and get a new shared_ptr given a qid:
        boost::mutex::scoped_lock lock(m_mut_results);
        if(!query_exists(qid)) return;
        cq = rq(qid);
        if(!cq || cq->cancelled()) return;
        // this disables callbacks and marks it as cancelled:
        cq->cancel();
        m_queries.erase(qid);
        // stop and cleanup timer:
        boost::asio::deadline_timer * t = m_qidtimers[qid];
        if(t)
        {
            t->cancel();
            delete(t);
            m_qidtimers.erase(qid);
        }
        // cleanup registered source ids -> playable items:
        vector< pi_ptr > results = cq->results();
        BOOST_FOREACH( pi_ptr pip, results )
        {
            m_pis.erase( pip->id() );
        }
    }
    // the RQ should not be referenced anywhere and will destruct now.
    // a resolverservice may still be processing it, in which case it will destruct once done.
}

/// called when timer expires, so we can delete stale queries
void
Resolver::cancel_query_timeout(query_uid qid)
{
    cout << "Stale timeout reached for QID: " << qid << endl;
    rq_ptr rq;
    {
        boost::mutex::scoped_lock lock(m_mut_results);
        if(!query_exists(qid)) return;
        rq = this->rq(qid);
        if(rq->cancelled()) return;
    }
    // check if it's stale enough to warrant cleaning up
    time_t now;
    time(&now);
    time_t diff = now - rq->atime();
    if( diff >= max_query_lifetime() ) // stale, clean it up
    {
        cancel_query( qid );
    }
    else if( m_qidtimers.find(qid) != m_qidtimers.end() ) // not stale, reset timer
    {
        cout << "Not stale, resetting timer." << endl;
        m_qidtimers[qid]->expires_from_now(boost::posix_time::seconds(max_query_lifetime()-diff));
        m_qidtimers[qid]->async_wait(boost::bind(&Resolver::cancel_query_timeout, this, qid));
    }
}

/// gets all the current results for a query
/// but leaves query active. (ie, results may change later)
vector< pi_ptr >
Resolver::get_results(query_uid qid)
{
    vector< pi_ptr > ret;
    boost::mutex::scoped_lock lock(m_mut_results);
    if(!query_exists(qid)) throw; // query was deleted
    return m_queries[qid]->results();
}

/// check how many results we found for this query id
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

/// does this qid still exist?
bool 
Resolver::query_exists(const query_uid & qid)
{
    return (m_queries.find(qid) != m_queries.end());
}

/// true on success, false if it already exists.
bool 
Resolver::add_new_query(boost::shared_ptr<ResolverQuery> rq)
{
    if(query_exists(rq->id())) return false;
    m_queries[rq->id()] = rq;
    m_qidlist.push_front(rq->id());
    return true;
}

boost::shared_ptr<ResolverQuery>
Resolver::rq(const query_uid & qid)
{
    return m_queries[qid];
}

size_t
Resolver::num_seen_queries()
{
    return m_queries.size();
}

boost::shared_ptr<PlayableItem> 
Resolver::get_pi(const source_uid & sid)
{
    return m_pis[sid];
}

