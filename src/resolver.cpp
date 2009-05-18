/*
    Playdar - music content resolver
    Copyright (C) 2009  Richard Jones
    Copyright (C) 2009  Last.fm Ltd.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "playdar/application.h"

#include <sstream>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/version.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "playdar/resolver.h"

#include "playdar/rs_script.h"

// Generic track calculation stuff:
#include "playdar/track_rq_builder.hpp"
#include "playdar/utils/levenshtein.h"
#include "playdar/pluginadaptor_impl.hpp"

// PDL stuff:
#include <DynamicLoader.hpp>
#include <DynamicClass.hpp>
#include <DynamicLibrary.hpp>
#include <DynamicLibraryManager.hpp>
#include <LoaderException.hpp>
#include "./platform.h"
// end PDL stuff

namespace playdar { 

using namespace resolvers;
using namespace std;

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
    
    // Initialize built-in curl SS facts:
    detect_curl_capabilities();

    // Load all non built-in resolvers:
    try
    {
        // Scripts resolvers:
        if( m_app->conf()->get<bool>("load_scripts", true) )
            load_resolver_scripts(); // external processes
        else 
            cerr << "NOT loading scripts, due to load_scripts=no" << endl;
        
        // PDL-loaded plugins:
        if( m_app->conf()->get<bool>("load_plugins", true) )
            load_resolver_plugins(); // DLL plugins
        else 
            cerr << "NOT loading scripts, due to load_scripts=no" << endl;
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
    boost::function<bool (const pa_ptr &, const pa_ptr&)> sortfun =
        boost::bind(&Resolver::pluginadaptor_sorter, this, _1, _2);
    sort(m_resolvers.begin(), m_resolvers.end(), sortfun);
    cout << endl;
    cout << "Loaded resolvers (" << m_resolvers.size() << ")" << endl;
    BOOST_FOREACH( pa_ptr & pa, m_resolvers )
    {
        cout << "RESOLVER w:" << pa->weight() << "\tp:" << pa->preference() 
             << "\tt:" << pa->targettime() 
             << "\t[" << (pa->script()?"script":"plugin") << "]  " 
             << pa->rs()->name() << endl;
    }
    cout << endl;
}

/// set up SS factories for every protocol curl can handle
void
Resolver::detect_curl_capabilities()
{
    curl_version_info_data * cv = curl_version_info(CURLVERSION_NOW);
    assert( cv->age >= 0 ); // should never get this far without curl.
    boost::function<boost::shared_ptr<CurlStreamingStrategy>(std::string)> 
     ssf = boost::bind( &Resolver::ss_ptr_generator<CurlStreamingStrategy>, this, _1 );
    const char * proto;
    for(int i = 0; (proto = cv->protocols[i]) ; i++ )
    {
        string p(proto);
        cout << "SS factory registered for: " << p << endl ;
        m_ss_factories[ p ] = ssf; // add an SS factory for this protocol
    }
}


Resolver::~Resolver()
{
    m_exiting = true;
    m_cond.notify_one();
    m_t->join();
    delete m_work;
    m_io_service->stop();
    m_iothr->join();
}

bool
Resolver::pluginadaptor_sorter(const pa_ptr& lhs, const pa_ptr& rhs)
{
    // this first check is really just cosmetic
    // the sorting looks nicer this way:
    if( lhs->weight() == rhs->weight() )
        return lhs->targettime() < rhs->targettime();
    // this is the important bit that orders the pipeline:
    return lhs->weight() > rhs->weight();
}

/// spawn resolver scripts:
void 
Resolver::load_resolver_scripts()
{
    using namespace boost::filesystem;
    
    path const etc = "scripts"; //FIXME don't depend on working directory
    cout << "Loading resolver scripts from: " << etc << endl;

    if (!exists(etc) || !is_directory(etc)) return;     // avoid the throw

    directory_iterator const end;
    string name;
    for(directory_iterator i(etc); i != end; ++i) {
        try {
#if BOOST_VERSION >= 103600
            string name = i->path().filename();
            string conf = "plugins." + i->path().stem() + '.';
#else
            string name = i->path().leaf();
            string conf = "plugins." + basename(i->path()) + '.';
#endif
            if (is_directory(i->status()) || is_other(i->status()))
                continue;
            //FIXME more sensible place to put resolving scripts
            if (name=="README.txt")
                continue;
            if (app()->conf()->get<bool>(conf+"enabled", true) == false){
                cout << "-> Skipping '"+name+"' - disabled in config file";
                continue;
            }

            cout << "-> Loading: " << name << endl;
            
            pa_ptr pap( new PluginAdaptorImpl( app()->conf(), this, name ) );
            pap->set_script( true );
            pap->set_scriptpath( i->path().string() );
            ResolverService * rs = new playdar::resolvers::rs_script();
            bool init = rs->init( pap );
            if(!init) throw 1;
            pap->set_rs( rs );
            pap->set_weight( app()->conf()->get<int>(conf+"weight",
                             rs->weight()) ); 
            pap->set_preference( app()->conf()->get<int>(conf+"preference",
                             rs->preference()) );
            pap->set_targettime( app()->conf()->get<int>(conf+"targettime",
                             rs->target_time()) );
            // if weight == 0, it doesnt resolve, but may handle HTTP calls etc.
            if(pap->weight() > 0) 
            {
                m_resolvers.push_back( pap );
                cout << "-> OK [w:" << pap->weight() 
                       << " p:" << pap->preference() 
                       << " t:" << pap->targettime() 
                       << "] " 
                 << pap->rs()->name() << endl;
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
        string confopt = "plugins.";
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
            ResolverServicePlugin * instance = 
                dynamicLoader.GetClassInstance< ResolverServicePlugin >
                    ( pluginfile.c_str(), classname.c_str() );
            pa_ptr pap( new PluginAdaptorImpl( app()->conf(), this, classname ) );
            if( ! instance->init(pap) )
            {
                cerr << "-> ERROR couldn't initialize." << endl;
                instance->Destroy();
                continue;
            }
            m_pluginNameMap[ boost::to_lower_copy(classname) ] = instance;
            cout << "Added pluginName " << boost::to_lower_copy(classname) << endl;
            pap->set_script( false );
            pap->set_rs( instance );
            string rsopt = "plugins."+classname;
            pap->set_weight( app()->conf()->get<int>(rsopt + ".weight", 
                                                instance->weight()) );
            pap->set_preference( app()->conf()->get<int>(rsopt + ".preference", 
                                                instance->preference()) );
            pap->set_targettime( app()->conf()->get<int>(rsopt + ".targettime", 
                                                    instance->target_time()) );
            m_resolvers.push_back( pap );
            cout << "-> OK [w:" << pap->weight() 
                       << " p:" << pap->preference() 
                       << " t:" << pap->targettime() 
                       << "] " 
                 << pap->rs()->name() << endl;
            // add any custom SS handlers for this plugin
            map< std::string, boost::function<ss_ptr(std::string)> > ssfacts = instance->get_ss_factories();
            typedef std::pair< std::string, boost::function<ss_ptr(std::string)> > sspair_t;
            BOOST_FOREACH( sspair_t sp, ssfacts )
            {
                cout << "-> Added SS factory for protocol '"<<sp.first<<"'" << endl;
                m_ss_factories[ sp.first ] = sp.second;
            }
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
    boost::mutex::scoped_lock lk(m_mutex);
    if(!add_new_query(rq))
    {
        // already running
        return rq->id();
    }
    if(cb) rq->register_callback(cb);

    // setup comet callback if the request has a valid comet session id
    const string& cometId(rq->comet_session_id());
    if (cometId.length()) {
        boost::mutex::scoped_lock cometlock(m_comets_mutex);
        std::map< std::string, rq_callback_t >::const_iterator it = m_comets.find(cometId);
        if (it != m_comets.end()) {
            rq->register_callback(it->second);
        }
    }

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
    unsigned short atweight = 0;
    unsigned int mintime = 0;
    bool started = false;
    BOOST_FOREACH( pa_ptr pap, m_resolvers )
    {
        if(pap->weight() >= lastweight) continue;
        if(!started)
        {
            atweight = pap->weight();
            mintime = pap->targettime();
            started = true;
            //cout << "Pipeline at weight: " << atweight << endl;
        }
        if(pap->weight() != atweight)
        {
            // we've dispatched to everything of weight "atweight"
            // and the shortest targettime at that weight is "mintime"
            // so schedule a callaback after mintime to carry on down the
            // chain and dispatch to the next lowest weighted resolver services.
            //cout << "Will continue pipeline after " << mintime << "ms." << endl;
            boost::shared_ptr<boost::asio::deadline_timer> 
                t(new boost::asio::deadline_timer( m_work->get_io_service() ));
            t->expires_from_now(boost::posix_time::milliseconds(mintime));
            // pass the timer pointer to the handler so it doesnt autodestruct:
            t->async_wait(boost::bind(&Resolver::run_pipeline_cont, this,
                                      rq, atweight, t));
            break;
        }
        if(pap->targettime() < mintime) mintime = pap->targettime();
        // dispatch to this resolver:
        //cout << "Pipeline dispatching to " << pap->rs()->name() 
        //     << " (lastweight: " << lastweight << ")" << endl;
        pap->rs()->start_resolving(rq);
    }
}

void
Resolver::run_pipeline_cont( rq_ptr rq, 
                        unsigned short lastweight,
                        boost::shared_ptr<boost::asio::deadline_timer> oldtimer)
{
    //cout << "Pipeline continues.." << endl;
    if(rq->solved())
    {
        //cout << "Bailing from pipeline: SOLVED @ lastweight: " << lastweight 
        //     << endl;
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
Resolver::add_results(query_uid qid, const vector< ri_ptr >& results, string via)
{
    cout << "add_results(" << results.size() << ", '"<< via << "')" << endl;
    if(results.size()==0)
    {
        return true;
    }
    boost::mutex::scoped_lock lock(m_mut_results);
    
    if(!query_exists(qid)) 
        return false; // query was deleted

    // add these new results to the ResolverQuery object
    BOOST_FOREACH(const ri_ptr rip, results)
    {
        rq_ptr rq = m_queries[qid];
        // resolver fixes the score using a standard algorithm
        // unless a non-zero score was specified by resolver.
        if(rip->score() < 0 &&
           TrackRQBuilder::valid( rq ) &&
           rip->has_json_value<string>( "artist" ) &&
           rip->has_json_value<string>( "track" )
          )
        {
            string reason;
            float score = calculate_score( rq, rip, reason );
            if( score == 0.0) 
                continue;
            rip->set_score( score );
        }
        
        m_queries[qid]->add_result(rip);

        // update map of source id -> playable item
        string sid = rip->id();
        if (sid.length()) {
            m_sid2ri[sid] = rip;
        }
        //cout << "Adding: ";
        //json_spirit::write( rip->get_json(), cout );
        //cout << endl; 
    }
    return true;
}


string 
Resolver::sortname(const string& name) 
{ 
    string data(name); 
    std::transform(data.begin(), data.end(), data.begin(), ::tolower); 
    boost::trim(data); 
    return data; 
}


/// caluclate score 0-1 based on how similar the names are.
/// string similarity algo that combines art,alb,trk from the original
/// query (rq) against a potential match (pi).
/// this is mostly just edit-distance, with some extra checks.
/// TODO albums are ignored atm.
float 
Resolver::calculate_score( const rq_ptr & rq, // query
                                  const ri_ptr & ri, // candidate
                                  string & reason )  // fail reason
{
    using namespace boost;
    // original names from the query:
    string o_art    = trim_copy(to_lower_copy(rq->param( "artist" ).get_str()));
    string o_trk    = trim_copy(to_lower_copy(rq->param( "track" ).get_str()));
    string o_alb    = trim_copy(to_lower_copy(rq->param( "album" ).get_str()));

    // names from candidate result:
    string art      = trim_copy(to_lower_copy(ri->json_value("artist", "" )));
    string trk      = trim_copy(to_lower_copy(ri->json_value("track", "")));
    string alb      = trim_copy(to_lower_copy(ri->json_value("album","")));
    // short-circuit for exact match
    if(o_art == art && o_trk == trk) return 1.0;
    // the real deal, with edit distances:
    unsigned int trked = playdar::utils::levenshtein( 
                                                     sortname(trk),
                                                     sortname(o_trk));
    unsigned int arted = playdar::utils::levenshtein( 
                                                     sortname(art),
                                                     sortname(o_art));
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
    BOOST_FOREACH( pa_ptr pap, m_resolvers )
    {
        pap->rs()->cancel_query( qid );
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
        vector< ri_ptr > results = cq->results();
        BOOST_FOREACH( ri_ptr rip, results )
        {
            m_sid2ri.erase( rip->id() );
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
vector< ri_ptr >
Resolver::get_results(query_uid qid)
{
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
    if (rq->id().length() == 0) {
        // create and assign an id to the request
        rq->set_id( gen_uuid() );
    } else if (query_exists(rq->id())) {
        return false;
    }

    m_queries[rq->id()] = rq;
    {
        boost::mutex::scoped_lock lock(m_mut_qidlist);
        m_qidlist.push_front(rq->id());
    }
    return true;
}

boost::shared_ptr<ResolverQuery>
Resolver::rq(const query_uid & qid)
{
    map< query_uid, rq_ptr >::iterator it = m_queries.find(qid);
    return it == m_queries.end() ? boost::shared_ptr<ResolverQuery>() : it->second;
}

size_t
Resolver::num_seen_queries()
{
    return m_queries.size();
}

/// this creates a SS from the URL in the ResolvedItem
/// it checks our map of protocol -> SS factory where protocol is the bit before the : in urls.
ss_ptr
Resolver::get_ss(const source_uid & sid)
{
    map< source_uid, ri_ptr >::iterator it = m_sid2ri.find(sid);
    if (it != m_sid2ri.end()) {
        ri_ptr rip( it->second );
        if( rip->url().empty() ) return ss_ptr();

        size_t offset = rip->url().find(':');
        if( offset == string::npos ) return ss_ptr();

        string p = rip->url().substr(0, offset);
        cout << "get a SS("<<p<<") for url: " << rip->url() << endl;

        std::map< std::string, boost::function<ss_ptr(std::string)> >::iterator itFac = 
            m_ss_factories.find(p);
        if (itFac != m_ss_factories.end())
            return itFac->second(rip->url());
    }
    return ss_ptr();
}

template <class T>
boost::shared_ptr<T>
Resolver::ss_ptr_generator(string url)
{
    return boost::shared_ptr<T>(new T(url));
}

bool
Resolver::create_comet_session(const std::string& sessionId, rq_callback_t cb)
{
    boost::mutex::scoped_lock cometlock(m_comets_mutex);
    // a new callback replaces the old callback
    // todo: can we terminate the old comet session via the callback?
    // todo: need a general mechanism to terminate (like when shutting down)
    // todo: limit the number of simultaneous comet sessions or make them non-blocking
    m_comets[sessionId] = cb;

    return true;
}

}
