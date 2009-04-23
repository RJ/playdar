#include "boffin.h"
#include "BoffinDb.h"
#include "RqlOpProcessor.h"
#include "parser/parser.h"
#include "RqlOp.h"
#include "BoffinSample.h"
#include "SampleAccumulator.h"
#include "SimilarArtists.h"

#include "playdar/utils/urlencoding.hpp"        // maybe this should be part of the plugin api then?
#include "playdar/resolved_item.h"
#include "playdar/library.h"
#include "playdar/playdar_request.h"
#include "BoffinRQUtil.h"

using namespace fm::last::query_parser;
using std::string;
using std::ostringstream;

static RqlOp root2op( const querynode_data& node )
{
   RqlOp op;
   op.isRoot = true;
   op.name = node.name;
   op.type = node.type;
   op.weight = node.weight;

   return op;
}


static RqlOp leaf2op( const querynode_data& node )
{
   RqlOp op;
   op.isRoot = false;

   if ( node.ID < 0 )
      op.name = node.name;
   else
   {
      ostringstream oss;
      oss << '<' << node.ID << '>';
      op.name = oss.str();
   }
   op.type = node.type;
   op.weight = node.weight;

   return op;
}


////////////////////////////////////////////////////////

using namespace playdar;

class TagCloudItem : public ResolvedItem
{
public:
    TagCloudItem(const std::string& name, float weight, int trackCount, const std::string& source)
        :m_name(name)
        ,m_weight(weight)
        ,m_trackCount(trackCount)
    {
        set_score(m_weight);
        set_source(source);
    }

    void create_json(json_spirit::Object &o) const
    {
        using namespace json_spirit;
        o.push_back( Pair("name", m_name) );
        o.push_back( Pair("count", m_trackCount) );        
    }

    static bool validator(const json_spirit::Object& o)
    {
        map<string, json_spirit::Value> m;
        obj_to_map(o, m);
        
        if( m.find("name") != m.end() && m.find("name")->second.type() == json_spirit::str_type &&
            m.find("score") != m.end() && m.find("score")->second.type() == json_spirit::real_type &&
            m.find("count") != m.end() && m.find("count")->second.type() == json_spirit::int_type )
        {
            return true;
        }
        
        return false;
    }

    static ri_ptr generator(const json_spirit::Object& o)
    {
        map<string, json_spirit::Value> m;
        obj_to_map(o, m);

        // source is optional (?)
        map<string, json_spirit::Value>::const_iterator it( m.find("source") );
        std::string source( it == m.end() ? "" : it->second.get_str() );

        return ri_ptr(new TagCloudItem(
            m.find("name")->second.get_str(), 
            m.find("score")->second.get_real(), 
            m.find("count")->second.get_int(),
            source ) );              // source not always essential/present.
    }


private:
    std::string m_name;
    float m_weight;
    int m_trackCount;
};


////////////////////////////////////////////////////////


boffin::boffin()
    : m_thread( 0 )
    , m_thread_stop( false )
{
}

boffin::~boffin() throw()
{
    if (m_thread) {
        m_thread_stop = true;
        m_queue_wake.notify_all();
        m_thread->join();
        delete m_thread;
        m_thread = 0;
    }
}

std::string 
boffin::name() const
{
    return "Boffin";
}

// return false to disable resolver
bool 
boffin::init( pa_ptr pap )
{
    m_pap = pap;
    m_thread = new boost::thread( boost::bind(&boffin::thread_run, this) );

    std::string playdarDb = pap->get<string>( "db", "collection.db" );
    std::string boffinDb = pap->get<string>( "plugins.boffin.db", "boffin.db" );

    m_db = boost::shared_ptr<BoffinDb>( new BoffinDb(boffinDb, playdarDb) );
    m_sa = boost::shared_ptr<SimilarArtists>( new SimilarArtists() );

    m_pap->register_resolved_item(&TagCloudItem::validator, &TagCloudItem::generator);
    return true;
}


void
boffin::queue_work(boost::function< void() > work)
{
    if (!m_thread_stop) 
    {
        {
            boost::lock_guard<boost::mutex> guard(m_queue_mutex);
            m_queue.push(work);
        }
        m_queue_wake.notify_one();
    }
}

boost::function< void() > 
boffin::get_work()
{
    boost::unique_lock<boost::mutex> lock(m_queue_mutex);
    while (m_queue.empty()) {
        m_queue_wake.wait(lock);
        // we might be trying to shutdown:
        if( m_thread_stop )
        {   
            return 0;
        }
    }
    boost::function< void() > result = m_queue.front();
    m_queue.pop();
    return result;
}

void
boffin::thread_run()
{
    cout << "boffin thread_run" << endl;
    try {
        boost::function< void() > fun;
        while (!m_thread_stop) {
            fun = get_work();
            if( fun && !m_thread_stop ) fun();
        }
    }
    catch (std::exception &e) {
        std::cout << "boffin::thread_run exception " << e.what();
    }
    cout << "boffin::thread_run exiting" << endl;
}

/// max time in milliseconds we'd expect to have results in.
unsigned int 
boffin::target_time() const
{
    // we may have zero results (if it's a query we can't handle), 
    // and we don't want to delay any other resolvers, so:
    return 50;
}

/// highest weighted resolverservices are queried first.
unsigned short 
boffin::weight() const
{
    return 80;
}

void 
boffin::start_resolving(boost::shared_ptr<ResolverQuery> rq)
{
    queue_work( boost::bind( &boffin::resolve, this, rq ) );
}


static
boost::shared_ptr<TagCloudItem> 
makeTagCloudItem(const boost::tuple<std::string, float, int>& in, const std::string& source)
{
    return boost::shared_ptr<TagCloudItem>(
        new TagCloudItem(in.get<0>(), in.get<1>(), in.get<2>(), source));
}


void
boffin::resolve(boost::shared_ptr<ResolverQuery> rq)
{
    // optional int value to limit the results, otherwise, default is 100
    int limit = (rq->param_exists("boffin_limit") && rq->param_type("boffin_limit") == json_spirit::int_type) ? rq->param("boffin_limit").get_int() : 100;

    if (rq->param_exists("boffin_rql") && rq->param_type("boffin_rql") == json_spirit::str_type) {
        parser p;
        if (p.parse(rq->param("boffin_rql").get_str())) {
            std::vector<RqlOp> ops;
            p.getOperations<RqlOp>(
                boost::bind(&std::vector<RqlOp>::push_back, boost::ref(ops), _1),
                &root2op, 
                &leaf2op);
            ResultSetPtr rqlResults( RqlOpProcessor::process(ops.begin(), ops.end(), *m_db, *m_sa) );

            // sample from the rqlResults into our SampleAccumulator:
            const int artist_memory = 4;
            SampleAccumulator sa(artist_memory);
            boffinSample(limit, *rqlResults, 
                boost::bind(&SampleAccumulator::pushdown, &sa, _1),
                boost::bind(&SampleAccumulator::result, &sa, _1));

            // look up results, turn them into a vector of json objects
            std::vector< json_spirit::Object > results;
            BOOST_FOREACH(const TrackResult& t, sa.get_results()) {
                pi_ptr pip = PlayableItem::create( m_db->db(), t.trackId );
                pip->set_score( t.weight );
                pip->set_source( m_pap->hostname() );
                pip->set_id( m_pap->gen_uuid() );
                results.push_back( pip->get_json() );
            }

            m_pap->report_results(rq->id(), results);
            return;
        } 
        parseFail(p.getErrorLine(), p.getErrorOffset());
     
    } else if (rq->param_exists("boffin_tags")) {
        typedef std::pair< json_spirit::Object, ss_ptr > result_pair;
        using namespace boost;

        shared_ptr< BoffinDb::TagCloudVec > tv(m_db->get_tag_cloud(limit));
        vector< json_spirit::Object > results;
        const std::string source( m_pap->hostname() );
        BOOST_FOREACH(const BoffinDb::TagCloudVecItem& tag, *tv) {
            results.push_back( makeTagCloudItem( tag, source )->get_json() );

        }
        cout << "Boffin will now report resuilts" << endl;
        m_pap->report_results(rq->id(), results);
        cout << "Reported.."<< endl;
    }

}


void
boffin::parseFail(std::string line, int error_offset)
{
    std::cout << "rql parse error at column " << error_offset << " of '" << line << "'\n";
}


// handler for HTTP reqs we are registerd for:
playdar_response 
boffin::authed_http_handler(const playdar_request* req, playdar::auth* pauth)
{
    if(req->parts().size() <= 1)
        return "This plugin has no web interface.";
    
    rq_ptr rq;
    if( req->parts()[1] == "tagcloud" )
    {
        rq = BoffinRQUtil::buildTagCloudRequest();
    }
    else if( req->parts()[1] == "rql" && req->parts().size() >= 2)
    {
        rq = BoffinRQUtil::buildRQLRequest( playdar::utils::url_decode( req->parts()[2] ) );
    }

    if( !rq )
        return "Error!";
    
    rq->set_from_name( m_pap->hostname() );
    
    query_uid qid = m_pap->dispatch( rq );
    
    using namespace json_spirit;
    Object r;
    r.push_back( Pair("qid", qid ));
    
    
    std::string s1, s2;
    if(req->getvar_exists("jsonp")){ // wrap in js callback
        s1 = req->getvar("jsonp") + "(";
        s2 = ");\n";
    }
    
    ostringstream os;
    write_formatted( r, os );
    return playdar_response( s1 + os.str() + s2, false );
}
