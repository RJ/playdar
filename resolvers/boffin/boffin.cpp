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

#include <boost/progress.hpp>
#include "sqlite3pp.h"
#include "boffin.h"
#include "BoffinDb.h"
#include "RqlOpProcessor.h"
#include "RqlDbProcessor.h"
#include "RqlOp.h"
#include "BoffinSample.h"
#include "SampleAccumulator.h"
#include "SimilarArtists.h"

#include "parser/parser.h"
#include "playdar/utils/urlencoding.hpp"        // maybe this should be part of the plugin api then?
#include "playdar/playdar_request.h"
#include "playdar/resolved_item.h"
#include "playdar/resolver_query.hpp"
#include "../local/library.h"
#include "../local/resolved_item_builder.hpp"

using namespace std;
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

using namespace playdar;

namespace TagCloudItem
{

    static ri_ptr createTagCloudItem(
        const std::string& name, 
        float weight, 
        int tracks, 
        int seconds,
        const std::string& source)
    {
        ri_ptr rip( new ResolvedItem );
        rip->set_json_value( "name", name );
        rip->set_json_value( "weight", weight );
        rip->set_json_value( "count", tracks );
        rip->set_json_value( "seconds", seconds );
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
        while (!m_thread_stop) {
            boost::function< void() > fun = get_work();
            if( fun && !m_thread_stop ) 
                fun();
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
makeTagCloudItem(const boost::tuple<std::string, float, int, int>& in, const std::string& source)
{
    return TagCloudItem::createTagCloudItem(
        in.get<0>(), 
        in.get<1>(), 
        in.get<2>(), 
        in.get<3>(), 
        source);
}

static
void
query_to_tagvec(boost::shared_ptr< BoffinDb::TagCloudVec > tv, sqlite3pp::query& qry)
{
    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); i++) {
        tv->push_back( i->get_columns<string, float, int, int>(0, 1, 2, 3) );
    }
}

static
void
query_to_summary(boost::tuple<int, int>& summary, sqlite3pp::query& qry)
{
    summary = qry.begin()->get_columns<int, int>(0, 1);
}

void
boffin::resolve(boost::shared_ptr<ResolverQuery> rq)
{
    if (rq->param_exists("boffin_tracks") && rq->param_type("boffin_tracks") == json_spirit::str_type) {
        parser p;
        if (p.parse(rq->param("boffin_tracks").get_str())) {
            std::vector<RqlOp> ops;
            p.getOperations<RqlOp>(
                boost::bind(&std::vector<RqlOp>::push_back, boost::ref(ops), _1),
                &root2op, 
                &leaf2op);
            ResultSetPtr rqlResults( RqlOpProcessor::process(ops.begin(), ops.end(), *m_db, *m_sa) );

            string hostname( m_pap->hostname() );   // cache this because we can have many many results

            const int reportingChunkSize = 100;      // report x at a time to the resolver
            std::vector< json_spirit::Object > results;
            results.reserve(reportingChunkSize);    // avoid vector resizing

            BOOST_FOREACH(const TrackResult& t, *rqlResults) {
                json_spirit::Object js;
                js.reserve(13);
                playdar::ResolvedItemBuilder::createFromFid( m_db->db(), t.trackId, js );
                js.push_back( json_spirit::Pair( "sid", m_pap->gen_uuid()) );
                js.push_back( json_spirit::Pair( "source", hostname) );
                js.push_back( json_spirit::Pair( "weight", t.weight) );
                results.push_back( js );

                if ((results.size() % reportingChunkSize) == 0) {
                    bool cancel = !m_pap->report_results(rq->id(), results);
                    results.clear();
                    results.reserve(reportingChunkSize);
                    if (cancel) break;
                }
            }
            if (results.size()) {
                m_pap->report_results(rq->id(), results);  // left-overs
            }
            return;
        } 
        parseFail(p.getErrorLine(), p.getErrorOffset());
     
    } else if (rq->param_exists("boffin_tags") && rq->param_type("boffin_tags") == json_spirit::str_type) {
        typedef std::pair< json_spirit::Object, ss_ptr > result_pair;
        using namespace boost;

        string rql( rq->param("boffin_tags").get_str() );

        shared_ptr< BoffinDb::TagCloudVec > tv;
        {
            progress_timer t;
            if (rql == "*") {
                tv = m_db->get_tag_cloud();
            } else {
                tv = shared_ptr< BoffinDb::TagCloudVec >( new BoffinDb::TagCloudVec );
                bool ok = RqlDbProcessor::parseAndProcess(
                    rql, 
                    "SELECT name, sum(weight), count(weight), sum(pd.file.duration) "
                    "FROM track_tag "
                    "INNER JOIN tag ON track_tag.tag = tag.rowid "
                    "INNER JOIN pd.file_join ON track_tag.track = pd.file_join.track "
                    "INNER JOIN pd.file ON pd.file_join.file = pd.file.id "
                    "WHERE track_tag.track IN ",
                    "GROUP BY tag.rowid",
                    *m_db, *m_sa,
                    boost::bind(&query_to_tagvec, tv, _1));
                if (!ok) {
                    cout << "Boffin rql parse error: " << rql << endl;
                } 
            }
            cout << "Boffin retrieved tagcloud..";
        }

        vector< json_spirit::Object > results;
        {
            progress_timer t;
            results.reserve(tv->size());
            const std::string source( m_pap->hostname() );
            BOOST_FOREACH(const BoffinDb::TagCloudVecItem& tag, *tv) {
                results.push_back( makeTagCloudItem( tag, source )->get_json() );
            }
            cout << "Boffin prepared results...";
        }

        {
            progress_timer t;
            m_pap->report_results(rq->id(), results);
            cout << "Boffin reported tagcloud..";
        }
    } else if (rq->param_exists("boffin_summary") && rq->param_type("boffin_summary") == json_spirit::str_type) {
        string rql( rq->param("boffin_summary").get_str() );

        boost::tuple<int, int> summary(-1, -1);     // numer of files, total duration
        {
            boost::progress_timer t;
            if (rql == "*") {
                summary = m_db->summary();
            } else {
                bool ok = RqlDbProcessor::parseAndProcess(
                    rql, 
                    "SELECT count(pd.file.duration), sum(pd.file.duration) "
                    "FROM pd.file_join "
                    "INNER JOIN pd.file ON pd.file_join.file = pd.file.id "
                    "WHERE pd.file_join.track IN ",
                    "",
                    *m_db, *m_sa,
                    boost::bind(&query_to_summary, boost::ref(summary), _1));
                if (!ok) {
                    cout << "Boffin rql parse error: " << rql << endl;
                } 
            }
        }

        if (summary.get<0>() != -1) {
            boost::progress_timer t;
            json_spirit::Object js;
            js.push_back( json_spirit::Pair( "source", m_pap->hostname()) );
            js.push_back( json_spirit::Pair( "files", summary.get<0>()) );
            js.push_back( json_spirit::Pair( "seconds", summary.get<1>()) );
            std::vector< json_spirit::Object > results;
            results.push_back( js );            
            m_pap->report_results(rq->id(), results);
            cout << "Boffin reported summary..";
        }
    }

}


void
boffin::parseFail(std::string line, int error_offset)
{
    std::cout << "rql parse error at column " << error_offset << " of '" << line << "'\n";
}


// handler for boffin HTTP reqs we are registered for:
//
// req->parts()[0] = "boffin"
//             [1] = "tagcloud", "summary" or "tracks"
//             [2] = rql (optional)
//
bool
boffin::authed_http_handler(const playdar_request& req, playdar_response& resp, playdar::auth& pauth)
{
    if(req.parts().size() <= 1) {
        return false;
    }

    const char* operation;
    if (req.parts()[1] == "tags") {
        operation = "boffin_tags";
    } else if (req.parts()[1] == "tracks") {
        operation = "boffin_tracks";
    } else if (req.parts()[1] == "summary") {
        operation = "boffin_summary";
    } else {
        resp = "Error!";
        return true;
    }

    rq_ptr rq(new playdar::ResolverQuery);

    string rql("*");
    if (req.parts().size() > 2)
        rql = playdar::utils::url_decode( req.parts()[2] );
    rq->set_param(operation, rql);

    if (req.getvar_exists("comet"))
        rq->set_comet_session_id(req.getvar("comet"));

    rq->set_from_name( m_pap->hostname() );
    rq->set_origin_local( true );
    
    if (req.getvar_exists("qid")) {
        string qid = req.getvar("qid");
        if (!m_pap->query_exists(qid)) {
            rq->set_id(qid);
        } else {
            cout << "WARNING - boffin request provided a QID, but that QID already exists as a running query. Assigning a new QID." << endl;
        }
    }

    query_uid qid = m_pap->dispatch( rq );
    
    using namespace json_spirit;
    Object r;
    r.push_back( Pair("qid", qid ));
    
    std::string s1, s2;
    if(req.getvar_exists("jsonp")) { // wrap in js callback
        s1 = req.getvar("jsonp") + "(";
        s2 = ");\n";
    }

    resp = playdar_response( s1 + write_formatted( r ) + s2, false );
    return true;
}

json_spirit::Object 
boffin::get_capabilities() const
{
    json_spirit::Object o;
    o.push_back( json_spirit::Pair( "plugin", name() ));
    o.push_back( json_spirit::Pair( "description", "Tag and RQL goodness."));
    return o;
}

