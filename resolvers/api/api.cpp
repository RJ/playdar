#include "api.h"
#include <boost/algorithm/string.hpp>
#include "playdar/playdar_request.h"
#include "playdar/track_rq_builder.hpp"

namespace playdar {
namespace resolvers {

using namespace std;

bool
api::init( pa_ptr pap )
{
    m_pap = pap;
    return true;
}

playdar_response 
api::anon_http_handler(const playdar_request* req)
{
    using namespace json_spirit;
    ostringstream response; 
    if(req->getvar("method") == "stat") {
        Object r;
        r.push_back( Pair("name", "playdar") );
        r.push_back( Pair("version", m_pap->playdar_version()) );
        r.push_back( Pair("authenticated", false) );
        write_formatted( r, response );
    }
    
    string retval;
    if(req->getvar_exists("jsonp")) // wrap in js callback
    {
        retval = req->getvar("jsonp");
        retval += "(" ;
        string s = response.str();
        //while((pos = s.find("\n"))!=string::npos) s.erase(pos,1);
        retval += s + ");\n";
    } 
    else 
    {
        retval = response.str();
    }
        
    playdar_response r(retval, false);
    return r;
}

playdar_response 
api::authed_http_handler(const playdar_request* req, playdar::auth* pauth)
{
    using namespace json_spirit;
    
    ostringstream response; 
    do
    {
        if(req->getvar("method") == "stat") 
        {
            Object r;
            r.push_back( Pair("name", "playdar") );
            r.push_back( Pair("version", m_pap->playdar_version()) );
            r.push_back( Pair("authenticated", true) );
            
            r.push_back( Pair("hostname", m_pap->hostname()) );
            //r.push_back( Pair("permissions", permissions) );
            //r.push_back( Pair("capabilities", "TODO") ); // might do something clever here later
            write_formatted( r, response );
            break;
        }
        
        if(req->getvar("method") == "resolve")
        {
            string artist = req->getvar_exists("artist") ? req->getvar("artist") : "";
            string album  = req->getvar_exists("album") ? req->getvar("album") : "";
            string track  = req->getvar_exists("track") ? req->getvar("track") : "";
            // create a new query and start resolving it:
            boost::shared_ptr<ResolverQuery> rq = TrackRQBuilder::build(artist, album, track);

            // was a QID specified? if so, use it:
            if(req->getvar_exists("qid"))
            {
                if( !m_pap->query_exists(req->getvar("qid")) )
                {
                    rq->set_id(req->getvar("qid"));
                }
                else 
                {
                    cout << "WARNING - resolve request provided a QID, but that QID already exists as a running query. Assigning a new QID." << endl;
                    // new qid assigned automatically if we don't provide one.
                }
            }
            if( !TrackRQBuilder::valid(rq) ) // usually caused by empty track name or something.
            {
                cout << "Tried to dispatch an invalid query, failing." << endl;
                playdar_response r("");
                r.set_response_code( 400 ); // bad_request
                return r;
            }
            rq->set_from_name(m_pap->hostname());
            query_uid qid = m_pap->dispatch(rq);
            Object r;
            r.push_back( Pair("qid", qid) );
            write_formatted( r, response );
        }
        else if(req->getvar("method") == "cancel")
        {
            query_uid qid = req->getvar("qid");
            m_pap->cancel_query(qid);
            // return something.. typically not checked, but easier to debug like this:
            response << "{ \"status\" : \"OK\" }";
        }
        else if(req->getvar("method") =="get_results" && req->getvar_exists("qid"))
        {
            if( !m_pap->query_exists( req->getvar("qid") ) )
            {
                cerr << "Error get_results(" << req->getvar("qid") << ") - qid went away." << endl;
                playdar_response r("");
                r.set_response_code( 404 ); // bad_request
                return r;
            }
            Object r;
            Array qresults;
            vector< ri_ptr > results = m_pap->get_results(req->getvar("qid"));
            BOOST_FOREACH(ri_ptr rip, results)
            {
                qresults.push_back( rip->get_json());
            }   
            r.push_back( Pair("qid", req->getvar("qid")) );
            r.push_back( Pair("refresh_interval", 1000) ); //TODO something better?
            r.push_back( Pair("query", m_pap->rq(req->getvar("qid"))->get_json()) );
            r.push_back( Pair("results", qresults) );
            
            write_formatted( r, response );
        }
        else
        {
            response << "FAIL";
        }

    }while(false);
    
    // wrap the JSON response in the javascript callback code:
    string retval;
    if(req->getvar_exists("jsonp")) // wrap in js callback
    {
        retval = req->getvar("jsonp");
        retval += "(" ;
        string s = response.str();
        //while((pos = s.find("\n"))!=string::npos) s.erase(pos,1);
        retval += s + ");\n";
    } 
    else 
    {
        retval = response.str();
    }
    playdar_response r(retval, false);
    r.add_header( "Content-Type", req->getvar_exists("jsonp") ?
                                  "text/javascript; charset=utf-8" :
                                  "text/plain; charset=utf-8" );
    return r;
}


} // resolvers
} // playdar
