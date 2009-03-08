#include "application/application.h"
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/thread/mutex.hpp>

#include "resolvers/resolver.h"

#include "resolvers/rs_local_library.h"
//#include "resolvers/rs_lan_udp.h"
//#include "resolvers/rs_http_playdar.h"
//#include "resolvers/rs_http_gateway_script.h"
//#include "resolvers/darknet/rs_darknet.h"
#include "library/library.h"

#include <DynamicLoader.hpp>
#include <DynamicClass.hpp>
#include <DynamicLibrary.hpp>
#include <DynamicLibraryManager.hpp>
#include <LoaderException.hpp>
#include <platform.h>

#include <boost/filesystem.hpp>

Resolver::Resolver(MyApplication * app)
{
    m_app = app;
    m_id_counter = 0;
    cout << "Resolver started." << endl;
    
    m_rs_local = 0;

    
    m_rs_local  = new RS_local_library();
    m_rs_local->init(app);
    
    load_resolvers();
    
/*
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
    */
    /*if(app->option<string>("resolver.gateway_http.enabled")=="yes")
    {
        m_rs_http_gateway_script = new RS_http_gateway_script(app);
    }*/
    
}

// dynamically load resolver plugins:
void 
Resolver::load_resolvers()
{
    namespace bfs = boost::filesystem;
    cout << "Loading resolver plugins from: ./plugins" << endl;
    bfs::directory_iterator end_itr;
    bfs::path p("./plugins/");
    for(bfs::directory_iterator itr( p ); itr != end_itr; ++itr)
    {
        if ( bfs::is_directory(itr->status()) ) continue;
        string pluginfile = itr->string();
        string classname = bfs::basename(pluginfile);
        cout << classname << endl;
        if(classname.substr(classname.length()-9)!=".resolver") continue;
        // strip .resolver
        classname = classname.substr(0,classname.length()-9);
        cout << classname << endl;
        // strip lib prefix:
        if(classname.substr(0,3)=="lib") classname = classname.substr(3);
        try
        {
            PDL::DynamicLoader & dynamicLoader = PDL::DynamicLoader::Instance();
            cout << "-> Trying: " << pluginfile << endl;
            ResolverService * instance = dynamicLoader.GetClassInstance< ResolverService >
                                            ( pluginfile.c_str(), classname.c_str() );
            instance->init(app());
            cout << "-> Loaded: " << instance->name() << endl;
            m_resolvers.push_back(instance);
        }
        catch( PDL::LoaderException & ex )
        {
            cerr << "-> Error: " << ex.what() << endl;
        }
    }
    cout << "Num Resolvers Loaded: " << m_resolvers.size() << endl;


/*
    using namespace boost::extensions;
    string library_path = "./plugin.so";
    shared_library lib(library_path);
    // Attempt to open the shared library.
    if (!lib.open()) {
        std::cerr << "Library failed to open: " << library_path << std::endl;
        return;
    }
    type_map types;
    if (!lib.call(types)) {
        std::cerr << "Function not found!" << std::endl;
        return;
    }
    std::map<std::string, factory<ResolverService, MyApplication *> >& factories(types.get());
    if (factories.empty()) {
        std::cerr << "No resolvers found found!" << std::endl;
        return;
    }    
    for (std::map<std::string, factory<ResolverService, MyApplication *> >::iterator it
         = factories.begin();
       it != factories.end(); ++it) 
    {
        std::cout << "Loading: " << it->first << std::endl;
        boost::scoped_ptr<ResolverService> current_resolver(it->second.create(m_app));
        std::cout << "Created: " << current_resolver->name() << endl;
    }*/
}

// start resolving! (non-blocking)
// returns a query_uid so you can check status of this query later
query_uid 
Resolver::dispatch(boost::shared_ptr<ResolverQuery> rq, bool local_only/* = false */) 
{
    if(!rq->valid())
    {
        throw;
    }
    if(!add_new_query(rq))
    {
        cout<< "RESOLVER: Not dispatching "<<rq->id()<<" - already running." << endl;
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
