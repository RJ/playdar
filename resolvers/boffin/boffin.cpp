#include "boffin.h"
#include "BoffinDb.h"
#include "RqlOpProcessor.h"
#include "parser/parser.h"
#include "RqlOp.h"
#include "BoffinSample.h"
#include "SampleAccumulator.h"
#include "SimilarArtists.h"

#include "playdar/resolved_item.h"
#include "playdar/library.h"

#include "BoffinRQUtil.h"

using namespace fm::last::query_parser;


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


class TagCloudItem : public ResolvedItem
{
public:
    TagCloudItem(const std::string& name, float weight, int trackCount)
        :m_name(name)
        ,m_weight(weight)
        ,m_trackCount(trackCount)
    {
        set_score(m_weight);
    }

    void create_json(json_spirit::Object &o) const
    {
        using namespace json_spirit;
        o.push_back( Pair("name", m_name) );
        o.push_back( Pair("count", m_trackCount) );        
    }

    static bool validator(const json_spirit::Object& o)
    {
        try {
            generator(o);
        } catch (...) {
            return false;
        }
        return true;
    }

    static ri_ptr generator(const json_spirit::Object& o)
    {
        map<string, json_spirit::Value> m;
        obj_to_map(o, m);
        
        return ri_ptr(new TagCloudItem(
            m.find("name")->second.get_str(), 
            m.find("score")->second.get_real(), 
            m.find("count")->second.get_int() ) );
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

std::string 
boffin::name() const
{
    return "Boffin";
}

// return false to disable resolver
bool 
boffin::init(playdar::Config* c, Resolver* r)
{
    m_conf = c;
    m_resolver = r;
    m_thread = new boost::thread( boost::bind(&boffin::thread_run, this) );

    std::string playdarDb = r->app()->library()->dbfilepath();
    std::string boffinDb = conf()->get<string>( "plugins.boffin.db", "boffin.db" );

    m_db = boost::shared_ptr<BoffinDb>( new BoffinDb(boffinDb, playdarDb) );
    m_sa = boost::shared_ptr<SimilarArtists>( new SimilarArtists() );

    r->register_resolved_item(&TagCloudItem::validator, &TagCloudItem::generator);
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
    }
    boost::function< void() > result = m_queue.front();
    m_queue.pop();
    return result;
}

void
boffin::drain_queue()
{
    boost::lock_guard<boost::mutex> guard(m_queue_mutex);
    while (!m_queue.empty()) {
        m_queue.pop();
    }
}

void
boffin::thread_run()
{
    try {
        while (!m_thread_stop) {
            get_work()();
        }
    } 
    catch (boost::thread_interrupted) {
        std::cout << "boffin::thread_run exiting normally";
    } 
    catch (std::exception &e) {
        std::cout << "boffin::thread_run exception " << e.what();
    }

    drain_queue();
}

void
boffin::stop()
{
    if (m_thread) {
        m_thread_stop = true;
        m_thread->interrupt();
        m_thread->join();
        delete m_thread;
        m_thread = 0;
    }
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
makeTagCloudItem(const boost::tuple<std::string, float, int>& in)
{
    return boost::shared_ptr<TagCloudItem>(
        new TagCloudItem(in.get<0>(), in.get<1>(), in.get<2>()) );
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

            // look up results, turn them into PlayableItems:
            std::vector< boost::shared_ptr<ResolvedItem> > playables;
            Library *library = resolver()->app()->library();
            assert(library);
            BOOST_FOREACH(const TrackResult& t, sa.get_results()) {
                pi_ptr pip = library->playable_item_from_fid( t.trackId );
                playables.push_back( pip );
            }

            report_results(rq->id(), playables, "Boffin");
        } 
        parseFail(p.getErrorLine(), p.getErrorOffset());
    } else if (rq->param_exists("boffin_tags")) {
        using namespace boost;

        shared_ptr< BoffinDb::TagCloudVec > tv(m_db->get_tag_cloud(limit));
        vector< shared_ptr<ResolvedItem> > results;
        BOOST_FOREACH(const BoffinDb::TagCloudVecItem& tag, *tv) {
            results.push_back( makeTagCloudItem(tag) );
        }
        report_results(rq->id(), results, "Boffin");
    }
}


void
boffin::parseFail(std::string line, int error_offset)
{
    std::cout << "rql parse error at column " << error_offset << " of '" << line << "'\n";
}


// default is empty, ie no http urls handle
vector<string> 
boffin::get_http_handlers()
{
    vector<string> h;
    h.push_back("boffin");
    return h;
}

// handler for HTTP reqs we are registerd for:
playdar_response 
boffin::http_handler( const playdar_request& req, playdar::auth * pauth)
{
    if(req.parts().size() <= 1)
        return "This plugin has no web interface.";
    
    rq_ptr rq;
    if( req.parts()[1] == "tagcloud" )
    {
        rq = BoffinRQUtil::buildTagCloudRequest();
    }
    else if( req.parts()[1] == "rql" && req.parts().size() >= 2)
    {
        rq = BoffinRQUtil::buildRQLRequest( req.parts()[2] );
    }

    if( !rq )
        return "Error!";
    
    rq->set_from_name( conf()->name() );
    
    query_uid qid = resolver()->dispatch( rq );
    
    using namespace json_spirit;
    Object r;
    r.push_back( Pair("qid", qid ));
    
    ostringstream os;
    write_formatted( r, os );
    return playdar_response( os.str(), false );
}
