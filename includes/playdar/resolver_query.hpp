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
#ifndef __RESOLVER_QUERY_H__
#define __RESOLVER_QUERY_H__

//#include "playdar/application.h"
#include "playdar/types.h"
#include "playdar/config.hpp"
#include "playdar/resolved_item.h"

#include "json_spirit/json_spirit.h"
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include "time.h"

namespace playdar {

// Represents a search query to resolve a particular track
// Contains results, as they are found

class ResolverQuery
{
public:
    ResolverQuery()
        : m_solved(false), m_cancelled(false)
    {
        // set initial "last access" time:
        time(&m_atime);
    }
    
    /// when was this query last "used"
    time_t atime() const
    {
        return m_atime;
    }

    
    virtual ~ResolverQuery()
    {
        //cout << "DTOR, resolver query: " << id() << " - " << str() << endl;
    }
    
    /// caluclate score 0-1 based on how similar the similarity
    /// between playable item and the original request.
    /// @return score between 0-1 or == 0 if there's an error
    /// @param reason will be set to the fail reason.
    virtual float calculate_score( const ri_ptr & pi,
                                  std::string & reason )
    {
        return 1.0f;
    }
    /// performs cleanup, query not needed any more.
    /// object should destruct soon after this is called, once any lingering references are released.
    void cancel()
    {
        m_cancelled = true;
        m_callbacks.clear();
        std::cout << "RQ::cancel() for " << id() << std::endl;
    }
    
    /// returns true if query cancelled and pending destruction. (hint, release your ptr)
    bool cancelled() const { return m_cancelled; }
    
    virtual json_spirit::Object get_json() const
    {
        time(&m_atime);
        using namespace json_spirit;
        Object j;
        j.push_back( Pair("_msgtype", "rq")   );
        j.push_back( Pair("qid",    id())     );

        for( std::map<std::string,Value>::const_iterator i = m_qryobj_map.begin();
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
        boost::shared_ptr<ResolverQuery> rq(new ResolverQuery);

        using namespace json_spirit;
        std::map<std::string,Value> qryobj_map;
        obj_to_map(qryobj, qryobj_map);
        rq->m_qryobj_map = qryobj_map;

        std::map<std::string,Value>::const_iterator it;
        std::map<std::string,Value>::const_iterator end( qryobj_map.end() );
        if((it = qryobj_map.find("qid")) != end) 
            rq->set_id( it->second.get_str() );
        if((it = qryobj_map.find("from_name")) != end) 
            rq->set_from_name( it->second.get_str() );
                    
        return rq;
    }
    
    void set_id(const query_uid& q) { m_uuid = q; }
    
    void set_from_name(const std::string& s) { m_from_name = s; }

    void set_comet_session_id(const std::string& s) { m_comet_session_id = s; }

    const query_uid& id() const
    { 
        return m_uuid; 
    }

    const std::string& comet_session_id() const
    {
        return m_comet_session_id;
    }

    size_t num_results() const
    {
        return m_results.size();
    }

    std::vector< ri_ptr > results()
    {
        time(&m_atime);
        // sort results on score/preference.
        // TODO this could be memoized
        boost::function
                    < bool 
                    (   const ri_ptr &, 
                        const ri_ptr &
                    ) > sortfun = 
                    boost::bind(&ResolverQuery::sorter, this, _1, _2);
        
        boost::mutex::scoped_lock lock(m_mut);
        sort(m_results.begin(), m_results.end(), sortfun);
        return m_results; 
    }

    bool sorter(const ri_ptr & lhs, const ri_ptr & rhs)
    {
        // if equal scores, prefer item with higher preference 
        // usually this indicates network reliability or user-configured preference
        if( lhs->score() == rhs->score() )
        {
            return lhs->preference() > rhs->preference();
        }
        return lhs->score() > rhs->score();
    }

    void add_result(ri_ptr rip) 
    { 
        //cout << "RQ.add_result: "<< pip->score() <<"\t"
        //     << pip->artist() << " - " << pip->track() << endl;
        {
            boost::mutex::scoped_lock lock(m_mut);
            m_results.push_back(rip); 
        }
        // decide if this result "solves" the query:
        // for now just assume score of 1 means solved.
        if(rip->score() == 1.0) 
        {
//            cout << "SOLVED " << id() << endl;   
            m_solved = true;
        }
        // fire callbacks:
        BOOST_FOREACH(rq_callback_t & cb, m_callbacks)
        {
            cb(id(), rip);
        }
    }
    
    void register_callback(rq_callback_t cb)
    {
        m_callbacks.push_back( cb );
    }
    
    bool solved()   const { return m_solved; }
    std::string from_name() const { return m_from_name;  }

    bool param_exists( const std::string& param ) const { return m_qryobj_map.find( param ) != m_qryobj_map.end(); }
    const json_spirit::Value& param( const std::string& param ) const { return m_qryobj_map.find( param )->second; }
    const json_spirit::Value_type param_type( const std::string& param ) const { return m_qryobj_map.find( param )->second.type(); }
    
    template<typename T>
    void set_param( const std::string& param, const T& value ){ m_qryobj_map[param] = value; }
    
    std::string str() const
    {
        std::ostringstream os;
        os << "{ ";
        
        std::pair<std::string,json_spirit::Value> i;
        BOOST_FOREACH( i, m_qryobj_map )
        {
            // This should return a pretty string
            // so discard internal data.
            if( i.first == "qid" ||
                i.first == "_msgtype" ||
                i.first == "from_name" ) continue;
            
            if( i.second.type() == json_spirit::str_type )
                os << i.first << ": " << i.second.get_str();
            else if( i.second.type() == json_spirit::real_type )
                os << i.first << ": " << i.second.get_real();
            else if( i.second.type() == json_spirit::int_type )
                os << i.first << ": " << i.second.get_int();
            else 
                continue;

            os << ", ";
        }
        os << " }";
        return os.str();
    }
    
    bool isValidTrack()
    {
        std::map<std::string,json_spirit::Value>::const_iterator end(m_qryobj_map.end());
        std::map<std::string,json_spirit::Value>::const_iterator it;
            
        return (it = m_qryobj_map.find( "artist" ), it != end) &&
            it->second.type() == json_spirit::str_type &&
            it->second.get_str().length() && 
            (it = m_qryobj_map.find( "track" ), it != end) && 
            it->second.type() == json_spirit::str_type &&
            it->second.get_str().length();
    }

protected:
    std::map<std::string,json_spirit::Value> m_qryobj_map;

private:
    std::vector< ri_ptr > m_results;
    std::string m_from_name;
    std::string m_comet_session_id;
        
    // list of functors to fire on new result:
    std::vector<rq_callback_t> m_callbacks;
    query_uid m_uuid;
    boost::mutex m_mut;
    // set to true once we get a decent result
    bool m_solved;
    // set to true if trying to cancel/delete this query (if so, don't bother working with it)
    bool m_cancelled;
    // last access time (used to know if this query is stale and can be deleted)
    // mutable: it's auto-updated to mark the atime in various places.
    mutable time_t m_atime; 

};

}

#endif

