static const bool DISABLE_AUTH=false;

#include "playdar/application.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "json_spirit/json_spirit.h"

#include <boost/foreach.hpp>
#include <moost/http/filesystem_request_handler.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#include "playdar/playdar_request_handler.h"
#include "playdar/playdar_request.h"
#include "playdar/library.h"
#include "playdar/resolver.h"
#include "playdar/track_rq_builder.hpp"

/*

    Known gaping security problem:
    not doing htmlentities() on user-provided data before rendering HTML
    so there are many script-injection possibilities atm.
    TODO write/find an htmlentities method and apply liberally.

*/

void 
playdar_request_handler::init(MyApplication * app)
{
    cout << "HTTP handler online." << endl;
    m_pauth = new playdar::auth(app->library()->dbfilepath());
    m_app = app;
    // built-in handlers:
    m_urlHandlers[ "" ] = boost::bind( &playdar_request_handler::handle_root, this, _1, _2 );
    m_urlHandlers[ "crossdomain.xml" ] = boost::bind( &playdar_request_handler::handle_crossdomain, this, _1, _2 );
    m_urlHandlers[ "auth_1" ] = boost::bind( &playdar_request_handler::handle_auth1, this, _1, _2 );
    m_urlHandlers[ "auth_2" ] = boost::bind( &playdar_request_handler::handle_auth2, this, _1, _2 );
    m_urlHandlers[ "shutdown" ] = boost::bind( &playdar_request_handler::handle_shutdown, this, _1, _2 );
    m_urlHandlers[ "settings" ] = boost::bind( &playdar_request_handler::handle_settings, this, _1, _2 );
    m_urlHandlers[ "queries" ] = boost::bind( &playdar_request_handler::handle_queries, this, _1, _2 );
    m_urlHandlers[ "stats" ] = boost::bind( &playdar_request_handler::serve_stats, this, _1, _2 );
    m_urlHandlers[ "static" ] = boost::bind( &playdar_request_handler::serve_static_file, this, _1, _2 );
    m_urlHandlers[ "serve" ] = boost::bind( &playdar_request_handler::handle_serve, this, _1, _2 );
    m_urlHandlers[ "sid" ] = boost::bind( &playdar_request_handler::handle_sid, this, _1, _2 );
    
    //Local Collection / Main API plugin callbacks:
    m_urlHandlers[ "quickplay" ] = boost::bind( &playdar_request_handler::handle_quickplay, this, _1, _2 );
    m_urlHandlers[ "api" ] = boost::bind( &playdar_request_handler::handle_api, this, _1, _2 );
    
    // handlers provided by plugins TODO ask plugin if/what they actually handle anything?
    BOOST_FOREACH( loaded_rs & lrs, *m_app->resolver()->resolvers() )
    {
        string name = lrs.rs->name();
        boost::algorithm::to_lower( name );
        m_urlHandlers[ name ] = boost::bind( &playdar_request_handler::handle_pluginurl, this, _1, _2 );
    }
}


string 
playdar_request_handler::sid_to_url(source_uid sid)
{
    string u = app()->conf()->httpbase();
    u += "/sid/" + sid;
    return u;
}


void 
playdar_request_handler::handle_request(const moost::http::request& req, moost::http::reply& rep)
{
    //TODO: Handle % encodings
    
    cout << "HTTP " << req.method << " " << req.uri << endl;
    rep.unset_streaming();
    
    string base = req.uri.substr(1, req.uri.find("/", 1)-1);
    boost::to_lower(base);
    cout << "Base: " << base << endl;
    HandlerMap::iterator handler = m_urlHandlers.find( base );
    if( handler != m_urlHandlers.end())
        handler->second( req, rep );
    else
    {
        rep = moost::http::reply::stock_reply(moost::http::reply::not_found);
        //rep.content = "make a valid request, kthxbye";
    } 
}


/// Phase 1 - this was opened in a popup from a button in the playdar 
///           toolbar on some site that needs to authenticate
void 
playdar_request_handler::handle_auth1( const playdar_request& req,
                                       moost::http::reply& rep)
{
    if( !req.getvar_exists("website") ||
        !req.getvar_exists("name") )
                return;

    map<string, string> vars;
    string filename = app()->conf()->get(string("www_root"), string("www")).append("/static/auth.html");
    string ftoken   = m_pauth->gen_formtoken();
    vars["<%URL%>"]="";
    if(req.getvar_exists("receiverurl"))
    {
        vars["<%URL%>"] = req.getvar("receiverurl");
    }
    vars["<%FORMTOKEN%>"]=ftoken;
    vars["<%WEBSITE%>"]=req.getvar("website");
    vars["<%NAME%>"]=req.getvar("name");
    serve_dynamic(rep, filename, vars);
}


void 
playdar_request_handler::handle_auth2( const playdar_request& req, moost::http::reply& rep )
{
    
    if( !req.postvar_exists("website") ||
        !req.postvar_exists("name") ||
        !req.postvar_exists("formtoken"))
    {
        rep = moost::http::reply::stock_reply(moost::http::reply::bad_request);
        return;
    }
    
    if(m_pauth->consume_formtoken(req.postvar("formtoken")))
    {
        string tok = playdar::Config::gen_uuid(); 
        m_pauth->create_new(tok, req.postvar("website"), req.postvar("name"));
        if( !req.postvar_exists("receiverurl") ||
            req.postvar("receiverurl")=="" )
        {
            map<string,string> vars;
            string filename = app()->conf()->get(string("www_root"), string("www")).append("/static/auth.na.html");
            vars["<%WEBSITE%>"]=req.postvar("website");
            vars["<%NAME%>"]=req.postvar("name");
            vars["<%AUTHCODE%>"]=tok;
            serve_dynamic(rep, filename, vars);
        }
        else
        {
            ostringstream os;
            os  << req.postvar("receiverurl")
            << ( strstr(req.postvar("receiverurl").c_str(), "?")==0 ? "?" : "&" )
            << "authtoken=" << tok
            << "#" << tok;
            rep = rep.stock_reply(moost::http::reply::moved_permanently); 
            rep.headers.resize(3);
            rep.headers[2].name = "Location";
            rep.headers[2].value = os.str();
        }
    }
    else
    {
        cerr << "Invalid formtoken, not authenticating" << endl;
        rep = rep.stock_reply(moost::http::reply::unauthorized); 
        rep.content = "Not Authorized";
    }
}

void 
playdar_request_handler::handle_crossdomain( const playdar_request& req,
                                             moost::http::reply& rep)
{
    ostringstream os;
    os  << "<?xml version=\"1.0\"?>" << endl
        << "<!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\">"
        << "<cross-domain-policy><allow-access-from domain=\"*\" /></cross-domain-policy>" << endl;
    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = os.str().length();
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = "text/xml";
    rep.content = os.str();
}

void 
playdar_request_handler::handle_root( const playdar_request& req,
                                      moost::http::reply& rep)
{
    ostringstream os;
    os  << "<h2>" << app()->conf()->name() << "</h2>"
        << "<p>"
        << "Your Playdar server is running! Websites and applications that "
        << "support Playdar will ask your permission, and then be able to "
        << "access music you have on your machine."
        << "</p>"

        << "<p>"
        << "For quick and dirty resolving, you can try constructing an URL like: <br/> "
        << "<code>" << app()->conf()->httpbase() << "/quickplay/ARTIST/ALBUM/TRACK</code><br/>"
        << "</p>"

        << "<p>"
        << "For the real demo that uses the JSON API, check "
        << "<a href=\"http://www.playdar.org/\">Playdar.org</a>"
        << "</p>"

        << "<p>"
        << "<h3>Resolver Pipeline</h3>"
        << "<table>"
        << "<tr style=\"font-weight: bold;\">"
        << "<td>Plugin Name</td>"
        << "<td>Weight</td>"
        << "<td>Target Time</td>"
        << "<td>Configuration</td>"
        << "</tr>"
        ;
    unsigned short lw = 0;
    bool dupe = false;
    int i = 0;
    string bgc="";
    BOOST_FOREACH(loaded_rs lrs, *app()->resolver()->resolvers())
    {
        if(lw == lrs.weight) dupe = true; else dupe = false;
        if(lw==0) lw = lrs.weight;
        if(!dupe) bgc = (i++%2==0) ? "lightgrey" : "" ;
        os  << "<tr style=\"background-color: " << bgc << "\">"
            << "<td>" << lrs.rs->name() << "</td>"
            << "<td>" << lrs.weight << "</td>"
            << "<td>" << lrs.targettime << "ms</td>";
        os << "<td>" ;

        string name = lrs.rs->name();
        boost::algorithm::to_lower( name );
        os << "<a href=\""<< name << "/config" <<"\">" << name << " config</a><br/> " ;

        os  << "</td></tr>" << endl;
    }
    os  << "</table>"
        << "</p>"
        ;
    serve_body(os.str(), rep);

}

void
playdar_request_handler::handle_pluginurl( const playdar_request& req,
                                           moost::http::reply& rep )
{
    //TODO: handle script resolver urls?
    cout << "pluginhandler: " << req.parts()[0] << endl;
    ResolverService* resolver = app()->resolver()->get_resolver( req.parts()[0] );

    if( resolver == 0 )
    {
        cout << "No plugin of that name found." << endl;
        rep = moost::http::reply::stock_reply(moost::http::reply::not_found);
        return;
    }

    serve_body( resolver->http_handler( req,
                                        m_pauth), rep );
}

void 
playdar_request_handler::handle_shutdown( const playdar_request& req,
                                          moost::http::reply& rep )
{
    app()->shutdown();
}

void 
playdar_request_handler::handle_settings( const playdar_request& req,
                                          moost::http::reply& rep )
{
    
    if( req.parts().size() == 1 || req.parts()[1] == "config" )
    {
        ostringstream os;
        os  << "<h2>Configuration</h2>"
            << "<p>"
            << "Config options are stored as a JSON object in the file: "
            << "<code>" << app()->conf()->filename() << "</code>"
            << "</p>"
            << "<p>"
            << "The contents of the file are shown below, to edit it use "
            << "your favourite text editor."
            << "</p>"
            << "<pre>"
            << app()->conf()->str()
            << "</pre>"
            ;
        serve_body(os.str(), rep);
    }
    else if( req.parts()[1] == "auth" )
    {
        ostringstream os;
        os  << "<h2>Authenticated Sites</h2>";
        typedef map<string,string> auth_t;
        if( req.getvar_exists("revoke"))
        {
            m_pauth->deauth(req.getvar("revoke"));
            os  << "<p style=\"font-weight:bold;\">"
                << "You have revoked access for auth-token: "
                << req.getvar("revoke")
                << "</p>";
        }
        vector< auth_t > v = m_pauth->get_all_authed();
        os  << "<p>"
            << "The first time a site requests access to your Playdar, "
            << "you'll have a chance to allow/deny it. You can see the list "
            << "of authenticated sites here, and delete any if necessary."
            << "</p>"
            << "<table style=\"width:95%\">" << endl
            <<  "<tr style=\"font-weight:bold;\">"
            <<   "<td>Name</td>"
            <<   "<td>Website</td>"
            <<   "<td>Auth Code</td>"
            <<   "<td>Options</td>"
            <<  "</tr>"
            << endl;
        int i = 0;
        string formtoken = m_pauth->gen_formtoken();
        BOOST_FOREACH( auth_t &m, v )
        {
            os  << "<tr style=\"background-color:" << ((i++%2==0)?"#ccc":"") << ";\">"
                <<  "<td>" << m["name"] << "</td>"
                <<  "<td>" << m["website"] << "</td>"
                <<  "<td>" << m["token"] << "</td>"
                <<  "<td><a href=\"/settings/auth/?revoke="  
                << m["token"] <<"\">Revoke</a>"
                <<  "</td>"
                << "</tr>";
        }
        os  << "</table>" << endl;
        serve_body( os.str(), rep );
    }
    else
        rep = moost::http::reply::stock_reply(moost::http::reply::not_found);
}

string 
playdar_request_handler::handle_queries_root(const playdar_request& req)
{
    if(req.postvar_exists( "cancel_query" ) && req.postvar("cancel_query").length() && 
       req.postvar_exists( "qid" ) && req.postvar("qid").length())
    {
        app()->resolver()->cancel_query( req.postvar("qid") );
    }

    size_t numqueries = app()->resolver()->qids().size();
    ostringstream os;
    os  << "<h2>Current Queries ("
        << numqueries <<")</h2>"

        << "<table>"
        << "<tr style=\"font-weight:bold;\">"
        << "<td>QID</td>"
        << "<td>Options</td>"
        << "<td>Artist</td>"
        << "<td>Album</td>"
        << "<td>Track</td>"
        << "<td>Origin</td>"
        << "<td>Results</td>"
        << "</tr>"
        ;
    int i  = 0;
    deque< query_uid>::const_iterator it =
        app()->resolver()->qids().begin();
    string bgc="";
    while(it != app()->resolver()->qids().end())
    {
        rq_ptr rq;
        try
        { 
            if( !TrackRQBuilder::valid( rq ))
                continue;
            
            rq = app()->resolver()->rq(*it); 
            bgc = (++i%2) ? "lightgrey" : "";
            os  << "<tr style=\"background-color: "<< bgc << "\">";
            if(!rq)
            {
             os << "<td colspan=\"7\"><i>cancelled query</i></td>";
            }
            else
            {
             os << "<td style=\"font-size:60%;\">" 
                << "<a href=\"/queries/"<< rq->id() <<"\">" 
                << rq->id() << "</a></td>"
                << "<td style=\"align:center;\">"
                << "<form method=\"post\" action=\"\" style=\"margin:0; padding:0;\">"
                << "<input type=\"hidden\" name=\"qid\" value=\"" << rq->id() << "\"/>"
                << "<input type=\"submit\" value=\"X\" name=\"cancel_query\" style=\"margin:0; padding:0;\" title=\"Cancel and invalidate this query\"/></form>"
                << "</td>"
                << "<td>" << rq->param( "artist" ).get_str() << "</td>"
                << "<td>" << rq->param( "album" ).get_str() << "</td>"
                << "<td>" << rq->param( "track" ).get_str() << "</td>"
                << "<td>" << rq->from_name() << "</td>"
                << "<td " << (rq->solved()?"style=\"background-color: lightgreen;\"":"") << ">" 
                << rq->num_results() << "</td>"
                ; 
            }
            os << "</tr>";
        } catch(...) { }
        it++;
    }  
    return os.str();
}

void 
playdar_request_handler::handle_queries( const playdar_request& req,
                                         moost::http::reply& rep )
{
    cout << "Query handler, parts: " << req.parts()[0] << ", " << req.parts()[1] << endl;
    do
    {
        if( req.parts().size() == 1 )
        {
            const string& s = handle_queries_root(req);
            serve_body( s, rep );
            break;
        }
        else if( req.parts().size() == 2 )
        {
           query_uid qid = req.parts()[1];
           rq_ptr rq = app()->resolver()->rq(qid);
            if(!rq || !TrackRQBuilder::valid( rq ))
           {
               rep = moost::http::reply::stock_reply(moost::http::reply::not_found);
               return;
           }
           vector< ri_ptr > results = rq->results();

           ostringstream os;
           os  << "<h2>Query: " << qid << "</h2>"
               
               << "<table>"
               << "<tr><td>Artist</td>"
               << "<td>" << rq->param( "artist" ).get_str() << "</td></tr>"
               << "<tr><td>Album</td>"
               << "<td>" << rq->param( "album" ).get_str() << "</td></tr>"
               << "<tr><td>Track</td>"
               << "<td>" << rq->param( "track" ).get_str() << "</td></tr>"
               << "</table>"

               << "<h3>Results (" << results.size() << ")</h3>"
               << "<table>"
               << "<tr style=\"font-weight:bold;\">"
               << "<td>SID</td>"
               << "<td>Artist</td>"
               << "<td>Album</td>"
               << "<td>Track</td>"
               << "<td>Dur</td>"
               << "<td>Kbps</td>"
               << "<td>Size</td>"
               << "<td>Source</td>"
               << "<td>Score</td>"
               << "</tr>"
               ;
           string bgc="";
           int i = 0;
           BOOST_FOREACH(ri_ptr ri, results)
           {
               pi_ptr pi = boost::dynamic_pointer_cast<PlayableItem>(ri);
               if( !pi ) continue;
               
               bgc = ++i%2 ? "lightgrey" : "";
               os  << "<tr style=\"background-color:" << bgc << "\">"
                   << "<td style=\"font-size:60%\">"
                   << "<a href=\"/sid/"<< pi->id() << "\">" 
                   << pi->id() << "</a></td>"
                   << "<td>" << pi->artist()   << "</td>"
                   << "<td>" << pi->album()    << "</td>"
                   << "<td>" << pi->track()    << "</td>"
                   << "<td>" << pi->duration() << "</td>"
                   << "<td>" << pi->bitrate()  << "</td>"
                   << "<td>" << pi->size()     << "</td>"
                   << "<td>" << pi->source()   << "</td>"
                   << "<td>" << pi->score()    << "</td>"
                   << "</tr>"
                   ;
           }
           os  << "</table>";
           serve_body( os.str(), rep );
           break;
        }
        else
        {
           rep = moost::http::reply::stock_reply(moost::http::reply::not_found);
        }
    }
    while( false );
}

/// serves file-id from library 
void 
playdar_request_handler::handle_serve( const playdar_request& req,
                                       moost::http::reply& rep )
{
    if(req.parts().size() != 2)
    {
        rep = moost::http::reply::stock_reply(moost::http::reply::bad_request );
        return;
    }
    
    int fid = atoi(req.parts()[1].c_str());
    if(fid) serve_track( rep, fid );
}

/// serves file based on SID
void 
playdar_request_handler::handle_sid( const playdar_request& req,
                                     moost::http::reply& rep )
{
    if( req.parts().size() != 2)
    {
        rep = moost::http::reply::stock_reply(moost::http::reply::bad_request );
        return;
    }
    
    source_uid sid = req.parts()[1];
    serve_sid( rep, sid);
}

/// quick hack method for playing a song, if it can be found:
///  /quickplay/The+Beatles//Yellow+Submarine
void
playdar_request_handler::handle_quickplay( const playdar_request& req, 
                                           moost::http::reply& rep )
{
    if( req.parts().size() != 4
       || !req.parts()[1].length() || !req.parts()[3].length() )
    {
        rep = moost::http::reply::stock_reply( moost::http::reply::bad_request );
        return;
    }
    
    string artist   = playdar_request::unescape(req.parts()[1]);
    string album    = req.parts()[2].length()?playdar_request::unescape(req.parts()[2]):"";
    string track    = playdar_request::unescape(req.parts()[3]);
    boost::shared_ptr<ResolverQuery> rq = TrackRQBuilder::build(artist, album, track);
    rq->set_from_name(app()->conf()->name());
    query_uid qid = app()->resolver()->dispatch(rq);
    // wait a couple of seconds for results
    boost::xtime time; 
    boost::xtime_get(&time,boost::TIME_UTC); 
    time.sec += 2;
    boost::thread::sleep(time);
    vector< ri_ptr > results = app()->resolver()->get_results(qid);
    
    if( !results.size() ) return;
    
    pi_ptr result = boost::dynamic_pointer_cast<PlayableItem>(results[0]);

    if( !result) return;

    json_spirit::Object ro = results[0]->get_json();
    cout << "Top result:" <<endl;
    json_spirit::write_formatted( ro, cout );
    cout << endl;
    string url = "/sid/";
    url += result->id();
    rep.headers.resize(3);
    rep.status = moost::http::reply::moved_temporarily;
    moost::http::header h;
    h.name = "Location";
    h.value = url;
    rep.headers[2] = h;
    rep.content = "";
}

void
playdar_request_handler::handle_api( const playdar_request& req, 
                                           moost::http::reply& rep )
{
    // Parse params from querystring:
    if( !req.getvar_exists("method"))
    {
        rep = rep.stock_reply(moost::http::reply::bad_request);
        return;
    }
    
    /// Auth stuff
    string permissions = "";
    if(req.getvar_exists("auth"))
    {
        string whom;
        if(m_pauth->is_valid(req.getvar("auth"), whom) || DISABLE_AUTH)
        {
            //cout << "AUTH: validated " << whom << endl;
            permissions = "*"; // allow all.
        }
        else
        {
            //cout << "AUTH: Invalid authtoken." << endl;
        }
    }
    else
    {
        //cout << "AUTH: no auth value provided." << endl;
    }

    
    handle_rest_api( req, rep, permissions);
}

//
// REST API that returns JSON
//

void 
playdar_request_handler::handle_rest_api(   const playdar_request& req,
                                            moost::http::reply& rep,
                                            string permissions)
{
    using namespace json_spirit;
    
    ostringstream response; 
    do
    {
        if(req.getvar("method") == "stat") {
            Object r;
            r.push_back( Pair("name", "playdar") );
            r.push_back( Pair("version", "0.1.0") );
            r.push_back( Pair("authenticated", permissions.length()>0 ) );
            //r.push_back( Pair("permissions", permissions) );
            //r.push_back( Pair("capabilities", "TODO") ); // might do something clever here later
            write_formatted( r, response );
            break;
        }
        
        // No other calls allowed unless authenticated!
        if(permissions.length()==0)
        {
            cerr << "Not authed, abort!" << endl;
            return;
        }
        
        if(req.getvar("method") == "resolve")
        {
            string artist = req.getvar("artist");
            string album  = req.getvar("album");
            string track  = req.getvar("track");
            // create a new query and start resolving it:
            boost::shared_ptr<ResolverQuery> rq = TrackRQBuilder::build(artist, album, track);

            // was a QID specified? if so, use it:
            if(req.getvar_exists("qid"))
            {
                if(!app()->resolver()->query_exists(req.getvar("qid")))
                {
                    rq->set_id(req.getvar("qid"));
                } else {
                    cout << "WARNING - resolve request provided a QID, but that QID already exists as a running query. Assigning a new QID." << endl;
                    // new qid assigned automatically if we don't provide one.
                }
            }
            if(!TrackRQBuilder::valid(rq)) // usually caused by empty track name or something.
            {
                cout << "Tried to dispatch an invalid query, failing." << endl;
                rep = moost::http::reply::stock_reply(moost::http::reply::bad_request);
                return;
            }
            rq->set_from_name(app()->conf()->name());
            query_uid qid = app()->resolver()->dispatch(rq);
            Object r;
            r.push_back( Pair("qid", qid) );
            write_formatted( r, response );
        }
        else if(req.getvar("method") == "cancel")
        {
            query_uid qid = req.getvar("qid");
            app()->resolver()->cancel_query(qid);
            // return something.. typically not checked, but easier to debug like this:
            response << "{ \"status\" : \"OK\" }";
        }
        else if(req.getvar("method") =="get_results" && req.getvar_exists("qid"))
        {
            if( !app()->resolver()->query_exists( req.getvar("qid") ) )
            {
                cerr << "Error get_results(" << req.getvar("qid") << ") - qid went away." << endl;
                rep = moost::http::reply::stock_reply(moost::http::reply::not_found);
                return;
            }
            Object r;
            Array qresults;
            vector< ri_ptr > results = app()->resolver()->get_results(req.getvar("qid"));
            BOOST_FOREACH(ri_ptr rip, results)
            {
                qresults.push_back( rip->get_json());
            }   
            r.push_back( Pair("qid", req.getvar("qid")) );
            r.push_back( Pair("refresh_interval", 1000) ); //TODO move to ResolveQuery
            r.push_back( Pair("query", app()->resolver()->rq(req.getvar("qid"))->get_json()) );
            r.push_back( Pair("results", qresults) );
            
            write_formatted( r, response );
        }
        else if(req.getvar("method") == "list_queries")
        {
            deque< query_uid>::const_iterator it =
                app()->resolver()->qids().begin();
            Array qlist;
            while(it != app()->resolver()->qids().end())
            {
                rq_ptr rq;
                try
                { 
                    Object obj;
                    rq = app()->resolver()->rq(*it);
                    if(!rq) continue;
                    obj.push_back( Pair("num_results", (int)rq->num_results()) ); 
                    obj.push_back( Pair("query", rq->get_json()) );
                    qlist.push_back( obj );
                } catch(...) { }
                it++;
            }
            // wrap that in an object, so we can add stats to it later
            Object o;
            o.push_back( Pair("queries", qlist) );
            write_formatted( o, response );
        }
        else if(req.getvar("method") == "list_artists")
        {
            vector< artist_ptr > artists = app()->library()->list_artists();
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
        }
        else if(req.getvar("method") == "list_artist_tracks" &&
                req.getvar_exists("artistname"))
        {
            Array qresults;
            artist_ptr artist = app()->library()->load_artist( req.getvar("artistname") );
            if(artist)
            {
                vector< track_ptr > tracks = app()->library()->list_artist_tracks(artist);
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
        }
        else
        {
            response << "FAIL";
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
        
    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = retval.length();
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = req.getvar_exists("jsonp") ? "text/javascript" : "text/plain";
    rep.content = retval;
}

void 
playdar_request_handler::handle_json_query(string query, const moost::http::request& req, moost::http::reply& rep)
{
    using namespace json_spirit;
    cout << "Handling JSON query:" << endl << query << endl;
    Value jval;
    do
    {
        if(!read( query, jval )) break;
        
        return;
        
    } while(false);
    
    cerr << "Failed to parse JSON" << endl;
    rep.content = "error";
    return;
}

void
playdar_request_handler::serve_body(string reply, moost::http::reply& rep)
{
    std::ostringstream r;
    r   << "<html><head><title>Playdar</title>"
        << "<style type=\"text/css\">"
        << "td { padding: 5px; }"
        << "</style></head><body>"
        << "<h1>Local Playdar Server</h1>"
        << "<a href=\"/\">Home</a>"
        << "&nbsp; | &nbsp;"
        << "<a href=\"/stats/\">Stats</a>"
        << "&nbsp; | &nbsp;"
        << "<a href=\"/settings/auth/\">Authentication</a>"
        << "&nbsp; | &nbsp;"
        << "<a href=\"/queries/\">Queries</a>"
        << "&nbsp; | &nbsp;"
        << "<a href=\"/settings/config/\">Configuration</a>"
        
        << "&nbsp; || &nbsp;"
        << "<a href=\"http://www.playdar.org/\" target=\"playdarsite\">Playdar.org</a>"
        
        << "<hr style=\"clear:both;\" />";

    r << reply;
    r << "\n</body></html>";

    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = r.str().length();
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = "text/html";
    rep.content = r.str(); 
}

void
playdar_request_handler::serve_stats(const moost::http::request& req, moost::http::reply& rep)
{
    std::ostringstream reply;
    reply   << "<h2>Local Library Stats</h2>"
            << "<table>"
            << "<tr><td>Num Files</td><td>" << app()->library()->num_files() << "</td></tr>\n"
            << "<tr><td>Artists</td><td>" << app()->library()->num_artists() << "</td></tr>\n"
            << "<tr><td>Albums</td><td>" << app()->library()->num_albums() << "</td></tr>\n"
            << "<tr><td>Tracks</td><td>" << app()->library()->num_tracks() << "</td></tr>\n"
            << "</table>"
            << "<h2>Resolver Stats</h2>"
            << "<table>"
            << "<tr><td>Num queries seen</td><td><a href=\"/queries/\">" 
            << app()->resolver()->num_seen_queries() 
            << "</a></td></tr>\n"
            << "</table>"
            ;
    serve_body(reply.str(), rep);
}

void
playdar_request_handler::serve_static_file(const moost::http::request& req, moost::http::reply& rep)
{
    moost::http::filesystem_request_handler frh;
    frh.doc_root("www");
    frh.handle_request(req, rep);
}

// Serves the music file based on a SID 
// (from a playableitem resulting from a query)
void
playdar_request_handler::serve_sid( moost::http::reply& rep, source_uid sid)
{
    cout << "Serving SID " << sid << endl;
    ri_ptr rip = app()->resolver()->get_ri(sid);
    
    pi_ptr pip = boost::dynamic_pointer_cast<PlayableItem>(rip);
    if(!pip)
    {
        cerr << "This SID does not exist or does not resolve to a playable item." << endl;
        rep = moost::http::reply::stock_reply(moost::http::reply::not_found );
        return;
    }
    cout << "-> PlayableItem: " << pip->artist() << " - " << pip->track() << endl;
    boost::shared_ptr<StreamingStrategy> ss = pip->streaming_strategy();
    cout << "-> " << ss->debug() << endl;
    rep.headers.resize(2);
    if(pip->mimetype().length())
    {
        
        rep.headers[1].name = "Content-Type";
        rep.headers[1].value = pip->mimetype();
    }
    if(pip->size())
    {
        rep.headers[0].name = "Content-Length";
        rep.headers[0].value = boost::lexical_cast<string>(pip->size());
    }
    // hand off the streaming strategy for the http server to do:
    rep.set_streaming(ss, pip->size());
    return;  
}


// serves a .html file from docroot, but string substitutes stuff in the vars map.
void
playdar_request_handler::serve_dynamic( moost::http::reply& rep,
                                        string filename,
                                        map<string,string> vars)
{
    ifstream ifs;
    ifs.open(filename.c_str(), ifstream::in);
    if(ifs.fail())
    {
        cerr << "FAIL" << endl;
        return;
    }
    typedef std::pair<string,string> pair_t;
    ostringstream os;
    string line;
    while(std::getline(ifs, line))
    {
        BOOST_FOREACH(pair_t item, vars)
        {
            boost::replace_all(line, item.first, item.second);
        }
        os << line << endl;
    }
    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = os.str().length();
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = "text/html";
    rep.content = os.str(); 
}

/*
    Serves a file based on fid from library.
    Might be useful if browsing your local library and you want
    to play tracks without searching and generating SIDs etc.
*/
void
playdar_request_handler::serve_track( moost::http::reply& rep, int tid)
{
    vector<int> fids = app()->library()->get_fids_for_tid(tid);
    int fid = fids[0];
    //lookup file path and content type etc
    string full_path    = app()->library()->get_field("file", fid, "path");
    string mimetype     = app()->library()->get_field("file", fid, "mimetype");
    string size         = app()->library()->get_field("file", fid, "size");

    cout << "Serving track id: "<<tid << " using file id: " << fid << " File: " << full_path << " size: " << size << endl;

    // Open the file to send back.
    std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
    if (!is)
    {
        rep = moost::http::reply::stock_reply(moost::http::reply::not_found);
        return;
    }

    // Fill out the reply to be sent to the client.
    rep.status = moost::http::reply::ok;
    char buf[512];
    while (is.read(buf, sizeof(buf)).gcount() > 0)
        rep.content.append(buf, is.gcount());
    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = size;
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = mimetype;
// just install mozilla-plugin-vlc and it will play in-browser.
//    rep.headers[2].name = "";
//    rep.headers[2].value = "";
//    rep.headers[3].name = "";
//    rep.headers[3].value = "";
//    header("Content-Disposition: attachment; filename=\"".$filename."\";" );
//    header("Content-Transfer-Encoding: binary");
}

