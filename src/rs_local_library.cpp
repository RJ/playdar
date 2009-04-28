#include "playdar/rs_local_library.h"

#include <boost/foreach.hpp>

#include "playdar/application.h"
#include "playdar/library.h"
#include "playdar/utils/uuid.h"
#include "playdar/utils/levenshtein.h"
#include "playdar/resolver.h"
#include "playdar/resolved_item_builder.hpp"

using namespace std;

namespace playdar { namespace resolvers {

/*
    I want to integrate the ngram2/l implementation done by erikf
    in moost here. This is a bit hacky, but gets the job done 99% for now.
    Specifically it currently doesnt know about words.. 
    so "title" and "title (LIVE)" aren't very similar due to large edit-dist.
*/
    
bool
RS_local_library::init(pa_ptr pap)
{
    m_pap = pap;
    
    m_exiting = false;
    cout << "Local library resolver: " << app()->library()->num_files() 
         << " files indexed." << endl;
    if(app()->library()->num_files() == 0)
    {
        cout << endl << "WARNING! You don't have any files in your database!"
             << "Run the scanner, then restart Playdar." << endl << endl;
    }
    // worker thread for doing actual resolving:
    m_t = new boost::thread(boost::bind(&RS_local_library::run, this));
    
    return true;
}

void
RS_local_library::start_resolving( rq_ptr rq )
{
    boost::mutex::scoped_lock lk(m_mutex);
    m_pending.push_front( rq );
    m_cond.notify_one();
}

/// thread that loops forever processing incoming queries:
void
RS_local_library::run()
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
        cout << "RS_local_library runner exiting." << endl;
    }
}

// 

/// this is some what fugly atm, but gets the job done for now.
/// it does the fuzzy library search using the ngram table from the db:
void
RS_local_library::process( rq_ptr rq )
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
        vector<int> fids = app()->library()->get_fids_for_tid(sp.id);
        BOOST_FOREACH(int fid, fids)
        {
            ri_ptr rip = ResolvedItemBuilder::createFromFid(*app()->library(), fid);
            rip->set_id( m_pap->gen_uuid() );
            rip->set_source( m_pap->hostname() );
            final_results.push_back( rip->get_json() );
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
RS_local_library::find_candidates(rq_ptr rq, unsigned int limit)
{ 
    vector<scorepair> candidates;
    float maxartscore = 0;
    
    //Ignore this request_query - nothing that this can resolve from.
    if( !rq->param_exists( "artist" ) ||
        !rq->param_exists( "track" ))
        return candidates;
    
    vector<scorepair> artistresults =
        app()->library()->search_catalogue("artist", rq->param( "artist" ).get_str());
    BOOST_FOREACH( scorepair & sp, artistresults )
    {
        if(maxartscore==0) maxartscore = sp.score;
        float artist_multiplier = (float)sp.score / maxartscore;
        float maxtrkscore = 0;
        vector<scorepair> trackresults = 
            app()->library()->search_catalogue_for_artist(sp.id, 
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

}}
