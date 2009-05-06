/*
    Playdar - music content resolver
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
#include "boffin.h"
#include "BoffinDb.h"
#include "RqlOpProcessor.h"
#include "RqlOpProcessorTagCloud.h"
#include "parser/parser.h"
#include "RqlOp.h"
#include "BoffinSample.h"
#include "SampleAccumulator.h"
#include "SimilarArtists.h"

#include "playdar/utils/urlencoding.hpp"        // maybe this should be part of the plugin api then?
#include "playdar/resolved_item.h"
#include "../local/library.h"
#include "../local/resolved_item_builder.hpp"
#include "playdar/playdar_request.h"
#include "BoffinRQUtil.h"

using namespace fm::last::query_parser;
using namespace std;

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

namespace TagCloudItem
{

    static ri_ptr createTagCloudItem(const std::string& name, float weight, int trackCount, const std::string& source)
    {
        ri_ptr rip( new ResolvedItem );
        rip->set_json_value( "name", name );
        rip->set_json_value( "weight", weight );
        rip->set_json_value( "count", trackCount );
        rip->set_json_value( "source", source );

        return rip;
    }

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
    return 100;
}

void 
boffin::start_resolving(boost::shared_ptr<ResolverQuery> rq)
{
    queue_work( boost::bind( &boffin::resolve, this, rq ) );
}


static
ri_ptr 
makeTagCloudItem(const boost::tuple<std::string, float, int>& in, const std::string& source)
{
    return TagCloudItem::createTagCloudItem(in.get<0>(), in.get<1>(), in.get<2>(), source);
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
            int sequence = 0;
            std::vector< json_spirit::Object > results;
            BOOST_FOREACH(const TrackResult& t, sa.get_results()) {
                ri_ptr rip = playdar::ResolvedItemBuilder::createFromFid( m_db->db(), t.trackId );
                rip->set_json_value( "seq", sequence++ );
                rip->set_source( m_pap->hostname() );
                rip->set_id( m_pap->gen_uuid() );
                results.push_back( rip->get_json() );
            }

            m_pap->report_results(rq->id(), results);
            return;
        } 
        parseFail(p.getErrorLine(), p.getErrorOffset());
     
    } else if (rq->param_exists("boffin_tags") && rq->param_type("boffin_tags") == json_spirit::str_type) {
        typedef std::pair< json_spirit::Object, ss_ptr > result_pair;
        using namespace boost;

        string rql( rq->param("boffin_tags").get_str() );

        shared_ptr< BoffinDb::TagCloudVec > tv;
        
        if (rql == "*") {
            tv = m_db->get_tag_cloud(limit);
        } else {
            parser p;
            if (p.parse(rql)) {
                std::vector<RqlOp> ops;
                p.getOperations<RqlOp>(
                    boost::bind(&std::vector<RqlOp>::push_back, boost::ref(ops), _1),
                    &root2op, 
                    &leaf2op);
                tv = RqlOpProcessorTagCloud::process(ops.begin(), ops.end(), *m_db, *m_sa);
            } else {
                parseFail(p.getErrorLine(), p.getErrorOffset());
                return;
            }
        }

        vector< json_spirit::Object > results;
        const std::string source( m_pap->hostname() );
        BOOST_FOREACH(const BoffinDb::TagCloudVecItem& tag, *tv) {
            results.push_back( makeTagCloudItem( tag, source )->get_json() );
        }
        cout << "Boffin will now report results" << endl;
        m_pap->report_results(rq->id(), results);
        cout << "Reported.."<< endl;
    }

}


void
boffin::parseFail(std::string line, int error_offset)
{
    std::cout << "rql parse error at column " << error_offset << " of '" << line << "'\n";
}


// handler for boffin HTTP reqs we are registerd for:
//
// req->parts()[0] = "boffin"
//             [1] = "tagcloud" or "rql"
//             [2] = rql (optional)
//
bool
boffin::authed_http_handler(const playdar_request& req, playdar_response& resp, playdar::auth* pauth)
{
    if(req.parts().size() <= 1)
        return "This plugin has no web interface.";
    
    rq_ptr rq;
    if( req.parts()[1] == "tagcloud" )
    {
        rq = BoffinRQUtil::buildTagCloudRequest(
            req.parts().size() > 2 ? 
                playdar::utils::url_decode( req.parts()[2] ) : 
                "*" );
    }
    else if( req.parts()[1] == "rql" && req.parts().size() > 2)
    {
        rq = BoffinRQUtil::buildRQLRequest( playdar::utils::url_decode( req.parts()[2] ) );
    }
    else
    {
        return false;
    }

    if( !rq )
    {
        resp = "Error!";
        return true;
    }
    
    rq->set_from_name( m_pap->hostname() );
    
    query_uid qid = m_pap->dispatch( rq );
    
    using namespace json_spirit;
    Object r;
    r.push_back( Pair("qid", qid ));
    
    
    std::string s1, s2;
    if(req.getvar_exists("jsonp")){ // wrap in js callback
        s1 = req.getvar("jsonp") + "(";
        s2 = ");\n";
    }
    
    ostringstream os;
    write_formatted( r, os );
    resp = playdar_response( s1 + os.str() + s2, false );
    return true;
}
