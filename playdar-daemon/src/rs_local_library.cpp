#include "playdar/application.h"
#include "playdar/rs_local_library.h"
#include "playdar/library.h"
#include <boost/foreach.hpp>


/*
    I want to integrate the ngram2/l implementation done by erikf
    in moost here. This is a bit hacky, but gets the job done 99% for now.
*/

    
void
RS_local_library::init(playdar::Config * c, Resolver * r)
{
    m_resolver  = r;
    m_conf = c;
    cout << "Local library resolver: " << app()->library()->num_files() << " files indexed." << endl;
    if(app()->library()->num_files() == 0)
    {
        cout << endl << "WARNING! You don't have any files in your database! Run the scanner, then restart Playdar." << endl << endl;
    }
    // worker thread for doing actual resolving:
    boost::thread t(boost::bind(&RS_local_library::run, this));
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
    rq_ptr rq;
    while(true)
    {
        {
            boost::mutex::scoped_lock lk(m_mutex);
            if(m_pending.size() == 0) m_cond.wait(lk);
            rq = m_pending.back();
            m_pending.pop_back();
        }
        process( rq );
    }
}

/// this is some what fugly atm, but gets the job done for now.
/// it does the fuzzy library search using the ngram table from the db:
void
RS_local_library::process( rq_ptr rq )
{
    cout << "Library resolver, searching: " << rq->str() << endl;
    query_uid qid = rq->id();
    Library * library = app()->library();
    vector<scorepair> candidates; 
    // first find suitable artists:
    float maxartscore = 0;
    vector<scorepair> artistresults = library->search_catalogue("artist", rq->artist());
    for(vector<scorepair>::const_iterator itart = artistresults.begin(); 
        itart!=artistresults.end(); ++itart)
    {
        if(maxartscore==0) maxartscore = (*itart).score;
        float artist_multiplier = (float)(*itart).score / maxartscore;
        vector<scorepair> trackresults;
        // HACK for track="*" wildcard:
        if(rq->track()=="*")
        {
            boost::shared_ptr<Artist> artp = library->load_artist((*itart).id);
            vector< boost::shared_ptr<Track> > alltracks = library->list_artist_tracks(artp);
            BOOST_FOREACH(boost::shared_ptr<Track> tp, alltracks)
            {
                scorepair sp;
                sp.id = tp->id();
                sp.score=1;
                trackresults.push_back(sp);
            }
        }
        else
        {   
            trackresults = library->search_catalogue_for_artist((*itart).id, "track", rq->track());
        }
        float maxtrkscore = 0;
        for(vector<scorepair>::const_iterator ittrk = trackresults.begin(); 
            ittrk!=trackresults.end(); ++ittrk)
        {
            if(maxtrkscore==0) maxtrkscore = (*ittrk).score;
            float track_multiplier = (float) (*ittrk).score / maxtrkscore;
            // naively combine two scores:
            float combined_score = artist_multiplier * track_multiplier;
            scorepair cand;
            cand.id = (*ittrk).id;
            cand.score = combined_score;
            // *  (length-eddist) / length
            
            candidates.push_back(cand);
        } 
    }
    // sort candidates by combined score, sanity check results
    sort(candidates.begin(), candidates.end(), sortbyscore()); 
    
    if(rq->mode() != "spamme" && rq->track()!="*") 
    {
        if(candidates.size()>10) candidates.resize(10);
    }
    //cout << "Library resolver found " << candidates.size() << " possible candidates" << endl;
    
    vector< boost::shared_ptr<PlayableItem> > final_results;
    BOOST_FOREACH(scorepair &sp, candidates)
    {
        string artid = library->get_field("track", sp.id, "artist");
        string artistname   = library->get_name("artist", atoi(artid.c_str()));
        string trackname    = library->get_name("track", sp.id);
        // check levenstein and discard crappy results
        
        unsigned int trked = MyApplication::levenshtein( 
                                Library::sortname(trackname),
                                Library::sortname(rq->track()));
        unsigned int arted = MyApplication::levenshtein( 
                                Library::sortname(artistname),
                                Library::sortname(rq->artist()));
        // HACK: wildcard hack for track titles, in lieu of better query engine:
        if(rq->track()=="*")
        {
            trked=0; // pretend all candidate tracks were exact match
        }
        
        float artedt = 1.5;
        float trkedt = 1.5;
        float albedt = 1.5;
        float cutoff = 0.5;
        
        if(rq->mode()=="spamme") // change tollerances
        {
            artedt = trkedt = albedt = 1.15;
            cutoff = 0.2;
        }
        
        bool failartist = false;
        bool failtrack  = false;
        bool failscore  = false;
        
        if( arted > (rq->artist().length()/artedt) )
        {
            failartist=true;
        }
        if( trked > (rq->track().length()/trkedt) )
        {
            failtrack=true;
        }
        
        float finalscore = 0.0;
        if( arted < rq->artist().length() &&
            trked < rq->track().length() )
        {
            finalscore =  ( (float)(rq->artist().length()-arted)/rq->artist().length() ) *
                            ( (float)(rq->track().length()-trked)/rq->track().length() ) ;
        }
        
        if(finalscore < cutoff)
        {
            failscore=true;
        }
        ostringstream report;
        if(rq->track()!="*")
        {
            /*
            report << "Candidate: " << arted << "/" << trked << ":" << finalscore;
            report << " '"<< artistname << "' - '" << trackname << "'"
             ; 
             */
        }
        if( failscore
            ||
            (rq->mode()=="spamme" && failartist && failtrack)
            ||
            (rq->mode()!="spamme" && (failartist || failtrack))
          )
        {
            if(rq->track()!="*")
            {
                /*
                cout << report.str() << " REJECTED: ";
                if(failartist) cout << "artist ";
                if(failtrack) cout << "track ";
                if(failscore) cout << "score ";
                cout << endl;
                */
            }
            continue;
        }
        else
        {
            if(rq->track()!="*"){
                //cout << report.str() << " ACCEPTED" << endl;
            }
        }
        
        // multiple files in our collection may have the same matching metadata
        // add them all to the results (some may be higher bitrate, different format etc)
        vector<int> fids = app()->library()->get_fids_for_tid(sp.id);
        BOOST_FOREACH(int fid, fids)
        {
            boost::shared_ptr<PlayableItem> pip = app()->library()->playable_item_from_fid(fid);
            pip->set_score(finalscore);
            pip->set_source(conf()->name());
            final_results.push_back( pip );
        }
    }
    if(final_results.size())
    {
        report_results(qid, final_results, name());
    }
    else
    {
        cout << "Library: No matches for: " << rq->str() << endl;
    }
}



