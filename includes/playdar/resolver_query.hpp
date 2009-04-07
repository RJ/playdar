#ifndef __RESOLVER_QUERY_H__
#define __RESOLVER_QUERY_H__

//#include "playdar/application.h"
#include "playdar/types.h"
#include "playdar/config.hpp"
#include "playdar/playable_item.hpp"

#include "json_spirit/json_spirit.h"
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>


// Represents a search query to resolve a particular track
// Contains results, as they are found
using namespace std;

class ResolverQuery
{
public:
    ResolverQuery()
        : m_solved(false), m_cancelled(false)
    {
    }

    
    virtual ~ResolverQuery()
    {
        cout << "DTOR, resolver query: " << id() << " - " << str() << endl;
    }
    
    virtual bool valid() const { return true; }
    
    /// caluclate score 0-1 based on how similar the similarity
    /// between playable item and the original request.
    /// @return score between 0-1 or == 0 if there's an error
    /// @param reason will be set to the fail reason.
    virtual float calculate_score( const pi_ptr & pi,
                                  string & reason )
    {
        return 1.0f;
    }
    /// performs cleanup, query not needed any more.
    /// object should destruct soon after this is called, once any lingering references are released.
    void cancel()
    {
        m_cancelled = true;
        m_callbacks.clear();
        cout << "RQ::cancel() for " << id() << endl;
    }
    
    /// returns true if query cancelled and pending destruction. (hint, release your ptr)
    bool cancelled() const { return m_cancelled; }
    
    virtual json_spirit::Object get_json() const
    {
        using namespace json_spirit;
        Object j;
        j.push_back( Pair("_msgtype", "rq")   );
        j.push_back( Pair("qid",    id())     );

        for( map<string,Value>::const_iterator i = m_qryobj_map.begin();
            i != m_qryobj_map.end(); i++ )
        {
            if( i->first != "_msgtype" &&
                i->first != "qid" &&
                i->first != "from_name" )
                j.push_back( Pair( i->first, i->second ));
        }
        
        j.push_back( Pair("solved",  solved())  );
        j.push_back( Pair("from_name",  from_name())  );
        return j;
    }
    
    static boost::shared_ptr<ResolverQuery> from_json(json_spirit::Object qryobj)
    {
        string qid, from_name;
        
        using namespace json_spirit;
        map<string,Value> qryobj_map;
        obj_to_map(qryobj, qryobj_map);
        
        if(qryobj_map.find("qid")!=qryobj_map.end()) 
            qid = qryobj_map["qid"].get_str();
        if(qryobj_map.find("from_name")!=qryobj_map.end()) 
            from_name = qryobj_map["from_name"].get_str();
                    
        boost::shared_ptr<ResolverQuery>    
            rq(new ResolverQuery);

        rq->m_qryobj_map = qryobj_map;
        
        if(qid.length())  rq->set_id(qid);
        if(from_name.length())  rq->set_from_name(from_name);
        return rq;
    }
    
    void set_id(query_uid q)
    {
        m_uuid = q;
    }
    
    void set_from_name(string s) { m_from_name = s; }
    
    // create uuid on demand if one wasn't specified by now:
    const query_uid & id() const
    { 
        if(!m_uuid.length())
        {
            m_uuid = playdar::Config::gen_uuid();
        }
        return m_uuid; 
    }

    size_t num_results() const
    {
        return m_results.size();
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
        //if(lhs->score() == rhs->score()) return lhs->preference() > rhs->preference();
        // TODO: the one that came from the resolverservice with the
        // highest weight should win if there is a tie.
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
        // fire callbacks:
        BOOST_FOREACH(rq_callback_t & cb, m_callbacks)
        {
            cb(id(), pip);
        }
    }
    
    void register_callback(rq_callback_t cb)
    {
        m_callbacks.push_back( cb );
    }
    
    bool solved()   const { return m_solved; }
    string from_name() const { return m_from_name;  }

    bool param_exists( string param ) const { return m_qryobj_map.find( param ) != m_qryobj_map.end(); }
    const string& param( string param ) const { return m_qryobj_map.find( param )->second.get_str(); }
    
    virtual string str() const
    {
        return "Unknown Query";
    }
    
protected:
    map<string,json_spirit::Value> m_qryobj_map;

private:
    vector< boost::shared_ptr<PlayableItem> > m_results;
    string      m_from_name;
    
    // list of functors to fire on new result:
    vector<rq_callback_t> m_callbacks;
    // mutable because created on-demand the first time it's needed
    mutable query_uid   m_uuid;
    boost::mutex m_mut;
    // set to true once we get a decent result
    bool m_solved;
    bool m_cancelled;

};

#endif

