#ifndef __RESOLVER_QUERY_H__
#define __RESOLVER_QUERY_H__

#include "application/application.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/algorithm/string.hpp>

#include "json_spirit/json_spirit.h"

// Represents a search query to resolve a particular track
// Contains results, as they are found
using namespace std;

class ResolverQuery
{
public:
    ResolverQuery(string art, string alb, string trk)
        : m_artist(art), m_album(alb), m_track(trk), m_mode("normal"), m_solved(false)
    {
        boost::trim(m_artist);
        boost::trim(m_album);
        boost::trim(m_track);
    }
    
    // is this a valid / well formed query?
    bool valid() const
    {
        return m_artist.length()>0 && m_track.length()>0;
    }

    ResolverQuery(){}

    ~ResolverQuery()
    {
        cout << "dtor, resolver query: " << id() << endl;
    }
    
    
    
    json_spirit::Object get_json() const
    {
        using namespace json_spirit;
        Object j;
        j.push_back( Pair("qid",    id())     );
        j.push_back( Pair("artist", artist()) );
        j.push_back( Pair("album",  album())  );
        j.push_back( Pair("track",  track())  );
        j.push_back( Pair("mode",  mode())  );
        j.push_back( Pair("solved",  solved())  );
        return j;
    }
    
    static boost::shared_ptr<ResolverQuery> from_json(json_spirit::Object qryobj)
    {
        string qid, artist, album, track, mode;
        
        using namespace json_spirit;
        map<string,Value> qryobj_map;
        obj_to_map(qryobj, qryobj_map);
        
        if(qryobj_map.find("artist")!=qryobj_map.end()) artist = qryobj_map["artist"].get_str();
        if(qryobj_map.find("track")!=qryobj_map.end()) track  = qryobj_map["track"].get_str();
        if(qryobj_map.find("album")!=qryobj_map.end()) album  = qryobj_map["album"].get_str();
        
        if(qryobj_map.find("qid")!=qryobj_map.end()) qid    = qryobj_map["qid"].get_str();
        if(qryobj_map.find("mode")!=qryobj_map.end()) mode   = qryobj_map["mode"].get_str();
        
        if(artist.length()==0 || track.length()==0) throw;
        
        boost::shared_ptr<ResolverQuery> rq(new ResolverQuery(artist, album, track));
        if(mode.length()) rq->set_mode(mode);
        if(qid.length())  rq->set_id(qid);
        return rq;
    }
    
    void set_id(query_uid q)
    {
        m_uuid = q;
    }
    
    void set_mode(string s){ m_mode = s; }
    
    // create uuid on demand if one wasn't specified by now:
    const query_uid & id() const
    { 
        if(!m_uuid.length())
        {
            m_uuid = MyApplication::gen_uuid();
        }
        return m_uuid; 
    }
    
    string str() const
    {
        string s = artist();
        s+=" - ";
        s+=album();
        s+=" - ";
        s+=track();
        return s;
    }

    vector< boost::shared_ptr<PlayableItem> > results()
    {
        // sort results on score/preference.
        boost::function
                    < bool 
                    (   const boost::shared_ptr<PlayableItem> &, 
                        const boost::shared_ptr<PlayableItem> &
                    ) > sortfun = 
                    boost::bind(&ResolverQuery::sorter, this, _1, _2);
                    
        boost::mutex::scoped_lock lock(m_mut);
        sort(m_results.begin(), m_results.end(), sortfun);
        return m_results; 
    }

    

    bool sorter(const boost::shared_ptr<PlayableItem> & lhs, const boost::shared_ptr<PlayableItem> & rhs)
    {
        // if equal scores, prefer item with higher preference (ie, network reliability)
        if(lhs->score() == rhs->score()) return lhs->preference() > rhs->preference();
        return lhs->score() > rhs->score();
    }

    void add_result(boost::shared_ptr<PlayableItem> pip) 
    { 
        //cout << "RQ.add_result: "<< pip->score() <<"\t"
        //     << pip->artist() << " - " << pip->track() << endl;
        {
            boost::mutex::scoped_lock lock(m_mut);
            m_results.push_back(pip); 
        }
        // decide if this result "solves" the query:
        // for now just assume score of 1 means solved.
        if(pip->score() == 1) 
        {
//            cout << "SOLVED " << id() << endl;   
            m_solved = true;
        }
    }
    bool solved()   const { return m_solved; }
    string artist() const { return m_artist; }
    string album()  const { return m_album; }
    string track()  const { return m_track; }
    string mode()   const { return m_mode;  }

    
    
    
private:
    vector< boost::shared_ptr<PlayableItem> > m_results;
    string      m_artist;
    string      m_album;
    string      m_track;
    
    string      m_mode; // can be set to "spamme" if you want crap matching but lots of results
    
    // mutable because created on-demand the first time it's needed, if not already set:
    mutable query_uid   m_uuid;
    
    boost::mutex m_mut;

    // set to true once we get a decent result
    bool m_solved;

};

#endif

