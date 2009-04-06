#ifndef __TRACK_RESOLVER_QUERY__
#define __TRACK_RESOLVER_QUERY__

#include "resolver_query.hpp"
#include "utils/levenshtein.h"
class TrackResolverQuery : public ResolverQuery {
public:
    TrackResolverQuery( string artist, string album, string track )
    {
        m_qryobj_map[ "artist" ] = artist;
        m_qryobj_map[ "album" ] = album;
        m_qryobj_map[ "track" ] = track;
    }
    
    
    // is this a valid / well formed query?
    bool valid() const
    {
        return param_exists( "artist" ) && param( "artist").length()>0 && 
               param_exists( "track" ) && param( "track" ).length()>0;
    }
    
    string str() const
    {
        string s = param( "artist" );
        s+=" - ";
        s+=param("album");
        s+=" - ";
        s+=param("track");
        return s;
    }
    
    /// caluclate score 0-1 based on how similar the names are.
    /// string similarity algo that combines art,alb,trk from the original
    /// query (rq) against a potential match (pi).
    /// this is mostly just edit-distance, with some extra checks.
    /// TODO albums are ignored atm.
    float 
    calculate_score( const pi_ptr & pi, // candidate
                     string & reason )  // fail reason
    {
        using namespace boost;
        // original names from the query:
        string o_art    = trim_copy(to_lower_copy(param( "artist" )));
        string o_trk    = trim_copy(to_lower_copy(param( "track" )));
        string o_alb    = trim_copy(to_lower_copy(param( "album" )));
        // names from candidate result:
        string art      = trim_copy(to_lower_copy(pi->artist()));
        string trk      = trim_copy(to_lower_copy(pi->track()));
        string alb      = trim_copy(to_lower_copy(pi->album()));
        // short-circuit for exact match
        if(o_art == art && o_trk == trk) return 1.0;
        // the real deal, with edit distances:
        unsigned int trked = playdar::utils::levenshtein( 
                                                         Library::sortname(trk),
                                                         Library::sortname(o_trk));
        unsigned int arted = playdar::utils::levenshtein( 
                                                         Library::sortname(art),
                                                         Library::sortname(o_art));
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
    
    
};

#endif //__TRACK_RESOLVER_QUERY__