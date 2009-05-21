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
#include "rs_local_library.h"
#include <boost/foreach.hpp>

#include "library.h"
#include "playdar/utils/levenshtein.h"
#include "resolved_item_builder.hpp"
#include "playdar/resolver_query.hpp"
#include "playdar/playdar_request.h"
#include "playdar/playdar_response.h"

using namespace std;

namespace playdar { namespace resolvers {

/*
    I want to integrate the ngram2/l implementation done by erikf
    in moost here. This is a bit hacky, but gets the job done 99% for now.
    Specifically it currently doesnt know about words.. 
    so "title" and "title (LIVE)" aren't very similar due to large edit-dist.
*/
    
bool
local::init(pa_ptr pap)
{
    m_pap = pap;
   
    m_library = new Library( pap->getstring( "db", "" ).get_str());

    m_exiting = false;
    cout << "Local library resolver: " << m_library->num_files() 
         << " files indexed." << endl;
    if(m_library->num_files() == 0)
    {
        cout << endl << "WARNING! You don't have any files in your database!"
             << "Run the scanner, then restart Playdar." << endl << endl;
    }
    // worker thread for doing actual resolving:
    m_t = new boost::thread(boost::bind(&local::run, this));
    
    return true;
}

void
local::start_resolving( rq_ptr rq )
{
    boost::mutex::scoped_lock lk(m_mutex);
    m_pending.push_front( rq );
    m_cond.notify_one();
}

// thread that loops forever processing incoming queries:
void
local::run()
{
    try
    {
        while(true)
        {
            rq_ptr rq;
            {
                boost::mutex::scoped_lock lk(m_mutex);
                if(m_pending.size() == 0) m_cond.wait(lk);
                if(m_exiting) break;
                rq = m_pending.back();
                m_pending.pop_back();
            }
            if(rq && !rq->cancelled())
            {
                process( rq );
            }
        }
    }
    catch(...)
    {
        cout << "local runner exiting." << endl;
    }
}

// 

/// this is some what fugly atm, but gets the job done for now.
/// it does the fuzzy library search using the ngram table from the db:
void
local::process( rq_ptr rq )
{
    vector< json_spirit::Object > final_results;
    // get candidates (rough potential matches):
    vector<scorepair> candidates = find_candidates(rq, 10);
    // now do the "real" scoring of candidate results:
    string reason; // for scoring debug.
    BOOST_FOREACH(scorepair &sp, candidates)
    {
        // multiple files in our collection may have matching metadata.
        // add them all to the results.
        vector<int> fids = m_library->get_fids_for_tid(sp.id);
        BOOST_FOREACH(int fid, fids)
        {
            json_spirit::Object js;
            js.reserve(12);
            ResolvedItemBuilder::createFromFid( *m_library, fid, js );
            js.push_back( json_spirit::Pair( "sid", m_pap->gen_uuid()) );
            js.push_back( json_spirit::Pair( "source", m_pap->hostname()) );
            final_results.push_back( js );
        }
    }
    if(final_results.size())
    {
        m_pap->report_results( rq->id(), final_results );
    }
}

/// Search library for candidates roughly matching the query.
/// This works with track ids and associated metadata. It's possible
/// that our library has many files for the same track id (ie, same metadata)
/// this is of no concern to this method.
///
/// First find suitable artists, then collect matching tracks for each artist.
vector<scorepair> 
local::find_candidates(rq_ptr rq, unsigned int limit)
{ 
    vector<scorepair> candidates;
    float maxartscore = 0;
    
    //Ignore this request_query - nothing that this can resolve from.
    if( !rq->param_exists( "artist" ) ||
        !rq->param_exists( "track" ))
        return candidates;
    
    vector<scorepair> artistresults =
        m_library->search_catalogue("artist", rq->param( "artist" ).get_str());
    BOOST_FOREACH( scorepair & sp, artistresults )
    {
        if(maxartscore==0) maxartscore = sp.score;
        float artist_multiplier = (float)sp.score / maxartscore;
        float maxtrkscore = 0;
        vector<scorepair> trackresults = 
            m_library->search_catalogue_for_artist(sp.id, 
                                                          "track",
                                                          rq->param( "track" ).get_str());
        BOOST_FOREACH( scorepair & sptrk, trackresults )
        {
            if(maxtrkscore==0) maxtrkscore = sptrk.score;
            float track_multiplier = (float) sptrk.score / maxtrkscore;
            // combine two scores:
            float combined_score = artist_multiplier * track_multiplier;
            scorepair cand;
            cand.id = sptrk.id;
            cand.score = combined_score;
            candidates.push_back(cand);
        } 
    }
    // sort candidates by combined score
    sort(candidates.begin(), candidates.end(), sortbyscore()); 
    if(limit > 0 && candidates.size()>limit) candidates.resize(limit);
    return candidates;
}

bool
local::authed_http_handler(const playdar_request& req, playdar_response& resp, playdar::auth& pauth) 
{ 
    using namespace json_spirit;
    ostringstream response;
    if( req.parts().size() > 1 &&
        req.parts()[1] == "list_artists" )
    {
        vector< artist_ptr > artists = m_library->list_artists();
        Array qresults;
        BOOST_FOREACH(artist_ptr artist, artists)
        {
            Object a;
            a.push_back( Pair("name", artist->name()) );
            qresults.push_back(a);
        }
        // wrap that in an object, so we can add stats to it later
        Object jq;
        jq.push_back( Pair("results", qresults) );
        write_formatted( jq, response );
    } else if(req.parts()[1] == "list_artist_tracks" && 
                req.getvar_exists("artistname")) 
    { 
        Array qresults; 
        artist_ptr artist = m_library->load_artist( req.getvar("artistname") ); 
        if(artist) 
        { 
            vector< track_ptr > tracks = m_library->list_artist_tracks(artist); 
            BOOST_FOREACH(track_ptr t, tracks) 
            { 
                Object a; 
                a.push_back( Pair("name", t->name()) ); 
                qresults.push_back(a); 
            } 
        } 
        // wrap that in an object, so we can cram in stats etc later 
        Object jq; 
        jq.push_back( Pair("results", qresults) ); 
        write_formatted( jq, response ); 
    } else {
        return false;
    }


    string retval;
    if( req.getvar_exists( "jsonp" ))
    {
        retval = req.getvar( "jsonp" );
        retval += "(" ;
        retval += response.str();
        retval += ");\n";
    }
    else
    {
        retval = response.str();
    }
    resp = playdar_response( retval, false );
    return true;
} 

bool 
local::anon_http_handler(const playdar_request& req, playdar_response& resp, playdar::auth&) 
{ 
   if( req.parts().size() > 1 &&
       (req.parts()[1] == "config" || req.parts()[1] == "stats") )
   {
       std::ostringstream reply; 
       reply   << "<h2>Local Library Stats</h2>" 
               << "<table>" 
                           << "<tr><td>Num Files</td><td>" << m_library->num_files() << "</td></tr>\n" 
                           << "<tr><td>Artists</td><td>" << m_library->num_artists() << "</td></tr>\n" 
                           << "<tr><td>Albums</td><td>" << m_library->num_albums() << "</td></tr>\n" 
                           << "<tr><td>Tracks</td><td>" << m_library->num_tracks() << "</td></tr>\n" 
               << "</table>";
       resp = reply.str();
       return true;
   }
   return false; 
}

json_spirit::Object 
local::get_capabilities() const
{
    json_spirit::Object o;
    o.push_back( json_spirit::Pair( "plugin", name() ));
    o.push_back( json_spirit::Pair( "description", "Resolve music tracks against your local library."));
    return o;
}

}}
