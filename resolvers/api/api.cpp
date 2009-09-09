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
#include "api.h"
#include <boost/algorithm/string.hpp>
#include "playdar/playdar_request.h"
#include "playdar/track_rq_builder.hpp"
#include "playdar/logger.h"

namespace playdar {
namespace resolvers {

using namespace std;

bool
api::init( pa_ptr pap )
{
    m_pap = pap;
    return true;
}

bool
api::anon_http_handler(const playdar_request& req, playdar_response& resp, playdar::auth& /*pauth*/)
{
    using namespace json_spirit;
    ostringstream response; 
    if(req.getvar_exists("method") && req.getvar("method") == "stat") {
        Object r;
        r.push_back( Pair("name", "playdar") );
        r.push_back( Pair("version", m_pap->playdar_version()) );
        r.push_back( Pair("authenticated", false) );
        r.push_back( Pair("hostname", m_pap->hostname()) );
        write_formatted( r, response );
    }
    else
    {
        return false;
    }
    
    string retval;
    if(req.getvar_exists("jsonp")) // wrap in js callback
    {
        retval = req.getvar("jsonp");
        retval += "(" ;
        string s = response.str();
        //while((pos = s.find("\n"))!=string::npos) s.erase(pos,1);
        retval += s + ");\n";
    } 
    else 
    {
        retval = response.str();
    }
        
    resp = playdar_response(retval, false);
    resp.add_header( "Content-Type", req.getvar_exists("jsonp") ?
                                "text/javascript; charset=utf-8" :
                                "application/json; charset=utf-8" );
    return true;
}

bool
api::authed_http_handler(const playdar_request& req, playdar_response& resp, playdar::auth& pauth)
{
    using namespace json_spirit;
    
    ostringstream response; 
    do
    {
        if(!req.getvar_exists("method"))
            return false;
        const string& method( req.getvar("method") );

        if(method == "stat") 
        {
            Object r;
            r.push_back( Pair("name", "playdar") );
            r.push_back( Pair("version", m_pap->playdar_version()) );
            r.push_back( Pair("authenticated", true) );
            r.push_back( Pair("hostname", m_pap->hostname()) );
            r.push_back( Pair("capabilities", m_pap->capabilities()) );
            write_formatted( r, response );
            break;
        }
        else if(method == "resolve")
        {
            string artist = req.getvar_exists("artist") ? req.getvar("artist") : "";
            string album  = req.getvar_exists("album") ? req.getvar("album") : "";
            string track  = req.getvar_exists("track") ? req.getvar("track") : "";

            // create a new query and start resolving it:
            boost::shared_ptr<ResolverQuery> rq = TrackRQBuilder::build(artist, album, track);

            // was a QID specified? if so, use it:
            if(req.getvar_exists("qid"))
            {
                if( !m_pap->query_exists(req.getvar("qid")) )
                {
                    rq->set_id(req.getvar("qid"));
                }
                else 
                {
                    log::info() << "WARNING - resolve request provided a QID, but that QID already exists as a running query. Assigning a new QID." << endl;
                    // new qid assigned automatically if we don't provide one.
                }
            }
            if(req.getvar_exists("comet"))
            {
                rq->set_comet_session_id(req.getvar("comet"));
            }
            if( !rq->isValidTrack() ) // usually caused by empty track name or something.
            {
                log::info() << "Tried to dispatch an invalid query, failing." << endl;
                playdar_response r("");
                r.set_response_code( 400 ); // bad_request
                resp = r;
                return true;
            }
            rq->set_from_name(m_pap->hostname());
            rq->set_origin_local( true );
            query_uid qid = m_pap->dispatch(rq);
            Object r;
            r.push_back( Pair("qid", qid) );
            write_formatted( r, response );
        }
        else if(method == "cancel")
        {
            query_uid qid = req.getvar("qid");
            m_pap->cancel_query(qid);
            // return something.. typically not checked, but easier to debug like this:
            response << "{ \"status\" : \"OK\" }";
        }
        else if(method =="get_results" && req.getvar_exists("qid"))
        {
            if( !m_pap->query_exists( req.getvar("qid") ) )
            {
                log::error() << "Error get_results(" << req.getvar("qid") << ") - qid went away." << endl;
                playdar_response r("");
                r.set_response_code( 404 ); // bad_request
                resp = r;
                return true;
            }
            Object r;
            Array qresults;
            vector< ri_ptr > results = m_pap->get_results(req.getvar("qid"));
            BOOST_FOREACH(ri_ptr rip, results)
            {
                qresults.push_back( rip->get_json());
            }   
            r.push_back( Pair("qid", req.getvar("qid")) );
            r.push_back( Pair("refresh_interval", 1000) ); //TODO something better?
            r.push_back( Pair("query", m_pap->rq(req.getvar("qid"))->get_json()) );
            r.push_back( Pair("results", qresults) );
            
            write_formatted( r, response );
        }
        else
        {
            return false;
        }

    }while(false);
    
    // wrap the JSON response in the javascript callback code:
    string retval;
    if(req.getvar_exists("jsonp")) // wrap in js callback
    {
        retval = req.getvar("jsonp");
        retval += "(" ;
        string s = response.str();
        //while((pos = s.find("\n"))!=string::npos) s.erase(pos,1);
        retval += s + ");\n";
    } 
    else 
    {
        retval = response.str();
    }
    resp = playdar_response(retval, false);
    resp.add_header( "Content-Type", req.getvar_exists("jsonp") ?
                                  "text/javascript; charset=utf-8" :
                                  "application/json; charset=utf-8" );
    return true;
}


} // resolvers
} // playdar
