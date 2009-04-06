#include "playdar/rs_script.h"
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>
/*
This resolver spawns an external process, typically a python/ruby script
which will do the actual resolving. Messages are passed to the script down
stdin, results expected from stdout of the script.

Messages sent via stdin/out are framed with a 4-byte integer (big endian) denoting the length of the message. Actual protocol msgs are JSON objects.
*/
namespace playdar {
namespace resolvers {

/*
    init() will spawn the external script and block until the script
    sends us a settings object, containing a name, weight and targettime.
*/
bool
rs_script::init(playdar::Config * c, Resolver * r, string script) 
{
    m_resolver  = r;
    m_conf = c;
    m_dead = false;
    m_exiting = false;
    m_weight = 1;
    m_targettime = 1000;
    m_got_settings = false;
    m_scriptpath = script;
    if(m_scriptpath=="")
    {
        cout << "No script path specified. gateway plugin failed." 
             << endl;
        m_name = "MISSING SCRIPT PATH";
        m_dead = true;
        m_weight = 0;
        return false;
    }
    else if(!boost::filesystem::exists(m_scriptpath))
    {
        cout << "-> FAILED - script doesn't exist at: " << m_scriptpath << endl;
        return false;
    }
    else
    {
        m_name = m_scriptpath; // should be overwritten by script settings
        cout << "Starting resolver process: "<<m_scriptpath << endl;
        // wait for script to send us a settings object:
        boost::mutex::scoped_lock lk(m_mutex_settings);
        init_worker(); // launch script
        cout << "-> Waiting for settings from script (5 secs)..." << endl;
        if(!m_got_settings) 
        {
            boost::xtime time; 
            boost::xtime_get(&time,boost::TIME_UTC); 
            time.sec += 5;
            m_cond_settings.timed_wait(lk, time);
            /* doesn't work on boost 1.35, but new way is this:
            m_cond_settings.timed_wait(lk, boost::posix_time::seconds(5));
            */
        }
        
        if(m_got_settings)
        {
            cout << "-> OK, script reports name: " << m_name << endl;
            // dispatcher thread:
            m_dt = new boost::thread(boost::bind(&rs_script::run, this));
        }
        else
        {
            cout << "-> FAILED - script didn't report any settings" << endl;
            m_dead = true;
            m_weight = 0; // disable us.
            return false;
        }
    }
    return true;
}

rs_script::~rs_script() throw()
{
    cout <<"DTOR Resolver script " << endl;
    m_exiting = true;
    m_cond.notify_all();
    if(m_dt) m_dt->join();
    m_os->close();
    if(m_t) m_t->join();
}

void
rs_script::start_resolving(rq_ptr rq)
{
    if(m_dead)
    {
        cerr << "Not dispatching to script:  " << m_scriptpath << endl;
        return;
    }
    //cout << "gateway dispatch enqueue: " << rq->str() << endl;
    {
        boost::mutex::scoped_lock lk(m_mutex);
        m_pending.push_front( rq );
    }
    m_cond.notify_one();
}

/// thread that loops forever dispatching to the script:
void
rs_script::run()
{
    try
    {
        rq_ptr rq;
        while(true)
        {
            {
                //cout << "Waiting on something" << endl;
                boost::mutex::scoped_lock lk(m_mutex);
                if(m_pending.size() == 0) m_cond.wait(lk);
                if(m_exiting || m_dead) break;
                rq = m_pending.back();
                m_pending.pop_back();
            }
            // dispatch query to script:
            {
                //cout << "Got " << rq->str() << endl;
                ostringstream os;
                write_formatted( rq->get_json(), os );
                string msg = os.str();
                boost::uint32_t len = htonl(msg.length());
                m_os->write( (char*)&len, 4 );
                m_os->write( msg.data(), msg.length() );
                *m_os << flush;
            }
        }
    }
    catch(...)
    {
        cout << "exception in rs_script runner." << endl;
    }
    cout << "rs_script dispatch runner ending" << endl;
}


void
rs_script::init_worker()
{
        std::vector<std::string> args;
        args.push_back("--playdar-mode");
        bp::context ctx;
        ctx.stdout_behavior   = bp::capture_stream();
        ctx.stdin_behavior    = bp::capture_stream();
        ctx.stderr_behavior   = bp::capture_stream();

        bp::child c = bp::launch(m_scriptpath, args, ctx);
        m_c = new bp::child(c);
        m_os = & c.get_stdin();
        m_t = new boost::thread( boost::bind(&rs_script::process_output, this) );
        m_e = new boost::thread( boost::bind(&rs_script::process_stderr, this) );
}
    
void
rs_script::process_stderr()
{
    bp::pistream &is = m_c->get_stderr();
    string line;
    while (!is.fail() && !is.eof() && getline(is, line))
    {
        cerr << name() << ":\t" << line << endl;
    }
}

// runs forever processing output of script
void 
rs_script::process_output()
{
    cout << "Gateway process_output started.." <<endl;
    using namespace json_spirit;
    bp::pistream &is = m_c->get_stdout();
    Value j;
    char buffer[4096];
    boost::uint32_t len;
    while (!is.fail() && !is.eof())
    {
        is.read( (char*)&len, 4 );
        if(is.fail() || is.eof()) break;
        len = ntohl(len);
        cout << "Incoming msg of length " << len << endl;
        if(len > sizeof(buffer))
        {
            cerr << "Gateway plugin aborting, payload too big" << endl;
            break;
        }
        is.read( (char*)&buffer, len );
        if(is.fail() || is.eof()) break;
        string msg((char*)&buffer, len);
        std::cout << "Msg: '" << msg << "'"<< endl;
        if(!read(msg, j) || j.type() != obj_type)
        {
            cerr << "Aborting, invalid JSON." << endl;
            break;
        }
        // expect a query:
        Object ro = j.get_obj();
        map<string,Value> rr;
        obj_to_map(ro,rr);    
        // msg will either be a query result, or a settings object
        if( rr.find("_msgtype")==rr.end() ||
            rr["_msgtype"].type() != str_type )
        {
            cerr << "No string _msgtype property of JSON object. error." << endl;
            continue;
        }
        
        string msgtype = rr["_msgtype"].get_str();
        
        // initial resolver settings being reported:
        if(msgtype == "settings")
        {
            if( rr.find("weight") != rr.end() &&
                rr["weight"].type() == int_type )
            {
                m_weight = rr["weight"].get_int();
                //cout << "Gateway setting w:" << m_weight << endl;
            }
            
            if( rr.find("targettime") != rr.end() &&
                rr["targettime"].type() == int_type )
            {
                m_targettime = rr["targettime"].get_int();
                //cout << "Gateway setting t:" << m_targettime << endl;
            }
            
            if( rr.find("name") != rr.end() &&
                rr["name"].type() == str_type )
            {
                m_name = rr["name"].get_str();
                //cout << "Gateway setting name:" << m_name << endl;
            }
            m_got_settings = true;
            m_cond_settings.notify_one();
            continue;
        }
        
        // a query result:
        if( msgtype == "results" &&
            rr.find("qid") != rr.end() && 
            rr["qid"].type() == str_type &&
            rr.find("results") != rr.end() && 
            rr["results"].type() == array_type )
        {
            query_uid qid = rr["qid"].get_str();
            Array resultsA = rr["results"].get_array();
            cout << "Got " << resultsA.size() << " results from script" << endl;
            vector< boost::shared_ptr<PlayableItem> > v;
            BOOST_FOREACH(Value & result, resultsA)
            {
                Object po = result.get_obj();
                boost::shared_ptr<PlayableItem> pip;
                pip = PlayableItem::from_json(po);
                cout << "Parserd pip from script: " << endl;
                write_formatted(  pip->get_json(), cout );
                map<string,Value> po_map;
                obj_to_map(po, po_map);
                string url   = po_map["url"].get_str();  
                cout << "url=" << url << endl;
                // we don't give this to the shared_ptr yet, because
                // we need to call a method specific to httpss, not
                // in the parent ss (to set headers):
                HTTPStreamingStrategy * httpss = new HTTPStreamingStrategy(url);
                try
                {
                    if( po_map.find("extra_headers")!=po_map.end() &&
                        po_map["extra_headers"].type() == array_type )
                    {
                        Array a = po_map["extra_headers"].get_array();
                        BOOST_FOREACH( Value &eh, a )
                        {
                            if(eh.type() != str_type) continue;
                            httpss->extra_headers().push_back(boost::trim_copy(eh.get_str()));
                        }
                    
                    }
                } 
                catch(...) { delete httpss; continue; }
                boost::shared_ptr<StreamingStrategy> s(httpss);
                pip->set_streaming_strategy(s);
                v.push_back(pip);
            }
            report_results(qid, v, name());
        }
    }
    cout << "Gateway plugin read loop exited" << endl;
    m_dead = true;
}


}}
