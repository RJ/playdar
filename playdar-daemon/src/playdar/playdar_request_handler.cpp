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
#include "playdar/library.h"
#include "playdar/resolver.h"

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
}


string 
playdar_request_handler::sid_to_url(source_uid sid)
{
    string u = app()->conf()->httpbase();
    u += "/sid/" + sid;
    return u;
}


/// parse a querystring or form post body into a variables map
int
playdar_request_handler::collect_params(const string & url, map<string,string> & vars)
{
    UriParserStateA state;
    UriQueryListA * queryList;
    UriUriA uri;
    state.uri = &uri;
    int qsnum; // how many pairs in querystring
    // basic uri parsing
    if (uriParseUriA(&state, url.c_str()) != URI_SUCCESS)
    {
        cerr << "FAILED TO PARSE QUERYSTRING" << endl;
        return -1;
    }
    
    if ( uriDissectQueryMallocA(&queryList, &qsnum, uri.query.first, 
            uri.query.afterLast) != URI_SUCCESS)
    {
        // this means valid but empty
        uriFreeUriMembersA(&uri);
        return 0;
    } 
    else
    {
        UriQueryListA * q = queryList;
        for(int j=0; j<qsnum; j++)
        {
            vars[q->key]= (q->value ? q->value : "");
            if(q->next) q = q->next;
        }
        if(queryList) uriFreeQueryListA(queryList);
        uriFreeUriMembersA(&uri);
        return vars.size();
    }
}

void 
playdar_request_handler::handle_request(const moost::http::request& req, moost::http::reply& rep)
{
    cout << "HTTP " << req.method << " " << req.uri << endl;
    rep.unset_streaming();
    
    map<string, string> getvars;
    map<string, string> postvars;
    // Parse params from querystring:
    if( collect_params( req.uri, getvars ) == -1 )
    {
        rep = rep.stock_reply(moost::http::reply::bad_request);
    }
    // Parse params from post body, for form submission
    if( req.content.length() && collect_params( string("/?")+req.content, postvars ) == -1 )
    {
        rep = rep.stock_reply(moost::http::reply::bad_request);
    }
    /// parse URL parts
    string url = req.uri.substr(0, req.uri.find("?"));
    vector<std::string> parts;
    boost::split(parts, url, boost::is_any_of("/"));
    // get rid of cruft from leading/trailing "/" and split:
    if(parts.size() && parts[0]=="") parts.erase(parts.begin());
    /// Auth stuff
    string permissions = "";
    if(getvars.find("auth") != getvars.end())
    {
        string whom;
        if(m_pauth->is_valid(getvars["auth"], whom))
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

    /// localhost/ - the playdar instance homepage on localhost
    if(url=="/")
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
            vector<string> urls = lrs.rs->get_http_handlers();
            if( urls.size() )
            {
                BOOST_FOREACH(string u, urls)
                {
                    os << "<a href=\""<< u <<"\">" << u << "</a><br/> " ;
                }
            }
            os  << "</td></tr>" << endl;
        }
        os  << "</table>"
            << "</p>"
            ;
         serve_body(os.str(), req, rep);
    }
    /// Show config file (no editor yet)
    else if(url=="/settings/config/")
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
        serve_body(os.str(), req, rep);
    }
    /// Phase 1 - this was opened in a popup from a button in the playdar 
    ///           toolbar on some site that needs to authenticate
    else if(url=="/auth_1/" && 
            getvars.find("website") != getvars.end() &&
            getvars.find("name") != getvars.end() )
    {
        map<string,string> vars;
        string filename = "www/static/auth.html";
        string ftoken   = m_pauth->gen_formtoken();
        vars["<%URL%>"]="";
        if(getvars.find("receiverurl") != getvars.end())
        {
            vars["<%URL%>"] = getvars["receiverurl"];
        }
        vars["<%FORMTOKEN%>"]=ftoken;
        vars["<%WEBSITE%>"]=getvars["website"];
        vars["<%NAME%>"]=getvars["name"];
        serve_dynamic(req, rep, filename, vars);
    }
    /// Phase 2  - Provided the formtoken is valid, the user has authenticated
    ///            and we should create a new authcode and pass it on to the 
    ///            originating domain, which will probably set it in a cookie.
    else if(url=="/auth_2/" &&
            postvars.find("website") != postvars.end() &&
            postvars.find("name") != postvars.end() &&
            postvars.find("formtoken") != postvars.end() )
    {
        if(m_pauth->consume_formtoken(postvars["formtoken"]))
        {
            string tok = playdar::Config::gen_uuid(); 
            m_pauth->create_new(tok, postvars["website"], postvars["name"]);
            if( postvars.find("receiverurl") == postvars.end() ||
                postvars["receiverurl"]=="" )
            {
                map<string,string> vars;
                string filename = "www/static/auth.na.html";
                vars["<%WEBSITE%>"]=postvars["website"];
                vars["<%NAME%>"]=postvars["name"];
                vars["<%AUTHCODE%>"]=tok;
                serve_dynamic(req, rep, filename, vars);
            }
            else
            {
                ostringstream os;
                os  << postvars["receiverurl"]
                    << ( strstr(postvars["receiverurl"].c_str(), "?")==0 ? "?" : "&" )
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
    /// show active queries:
    else if(url=="/queries/")
    {
        size_t numqueries = app()->resolver()->qids().size();
        ostringstream os;
        os  << "<h2>Current Queries ("
            << numqueries <<")</h2>"
        
            << "<table>"
            << "<tr style=\"font-weight:bold;\">"
            << "<td>QID</td>"
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
                rq = app()->resolver()->rq(*it); 
                bgc = (++i%2) ? "lightgrey" : "";
                os  << "<tr style=\"background-color: "<< bgc << "\">"
                    << "<td style=\"font-size:60%;\">" 
                    << "<a href=\"/queries/"<< rq->id() <<"\">" 
                    << rq->id() << "</a></td>"
                    << "<td>" << rq->artist() << "</td>"
                    << "<td>" << rq->album() << "</td>"
                    << "<td>" << rq->track() << "</td>"
                    << "<td>" << rq->from_name() << "</td>"
                    << "<td " << (rq->solved()?"style=\"background-color: lightgreen;\"":"") << ">" 
                     << rq->num_results() << "</td>"
                    << "</tr>"
                    ; 
            } catch(...) { }
            it++;
        }
        serve_body( os.str(), req, rep );
    }
    /// Inspect specific query id:
    else if(parts[0]=="queries" && parts.size() == 2)
    {
        query_uid qid = parts[1];
        rq_ptr rq = app()->resolver()->rq(qid);
        if(!rq)
        {
            rep = moost::http::reply::stock_reply(moost::http::reply::not_found);
            return;
        }
        vector< pi_ptr > results = rq->results();
        
        ostringstream os;
        os  << "<h2>Query: " << qid << "</h2>"
            << "<table>"
            << "<tr><td>Artist</td>"
            << "<td>" << rq->artist() << "</td></tr>"
            << "<tr><td>Album</td>"
            << "<td>" << rq->album() << "</td></tr>"
            << "<tr><td>Track</td>"
            << "<td>" << rq->track() << "</td></tr>"
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
        BOOST_FOREACH(pi_ptr pi, results)
        {
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
        serve_body( os.str(), req, rep );
    }
    /// Shows a list of every authenticated site
    /// with a "revoke" options for each.
    else if(url=="/settings/auth/")
    {
        ostringstream os;
        os  << "<h2>Authenticated Sites</h2>";
        typedef map<string,string> auth_t;
        if( getvars.find("revoke")!=getvars.end() )
        {
            m_pauth->deauth(getvars["revoke"]);
            os  << "<p style=\"font-weight:bold;\">"
                << "You have revoked access for auth-token: "
                << getvars["revoke"]
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
        serve_body( os.str(), req, rep );
    }
    /// this is for serving static files, not sure we need it:
    else if(parts[0]=="static") 
    {
        serve_static_file(req, rep);
    }
    /// JSON API:
    else if(url=="/api/" && getvars.count("method")==1)
    {
        handle_rest_api(getvars, req, rep, permissions);
    }
    /// Misc stats on your playdar instance
    else if(url=="/stats/") 
    {
        serve_stats(req, rep);
    }
    /// quick hack method for playing a song, if it can be found:
    ///  /quickplay/The+Beatles//Yellow+Submarine
    else if(parts[0]=="quickplay" && parts.size() == 4
            && parts[1].length() && parts[3].length())
    {
        
        string artist   = unescape(parts[1]);
        string album    = parts[2].length()?unescape(parts[2]):"";
        string track    = unescape(parts[3]);
        boost::shared_ptr<ResolverQuery> rq(new ResolverQuery(artist, album, track));
        rq->set_from_name(app()->conf()->name());
        query_uid qid = app()->resolver()->dispatch(rq);
        // wait a couple of seconds for results
        boost::xtime time; 
        boost::xtime_get(&time,boost::TIME_UTC); 
        time.sec += 2;
        boost::thread::sleep(time);
        vector< boost::shared_ptr<PlayableItem> > results = app()->resolver()->get_results(qid);
        if(results.size())
        {
            json_spirit::Object ro = results[0]->get_json();
            cout << "Top result:" <<endl;
            json_spirit::write_formatted( ro, cout );
            cout << endl;
            string url = "/sid/";
            url += results[0]->id();
            rep.headers.resize(3);
            rep.status = moost::http::reply::moved_temporarily;
            moost::http::header h;
            h.name = "Location";
            h.value = url;
            rep.headers[2] = h;
            rep.content = "";
        }
    }
    /// serves file-id from library 
    else if(parts[0]=="serve" && parts.size() == 2)
    {
        int fid = atoi(parts[1].c_str());
        if(fid) serve_track(req, rep, fid);
    }
    /// serves file based on SID
    else if(parts[0]=="sid" && parts.size() == 2)
    {
        source_uid sid = parts[1];
        serve_sid(req, rep, sid);
    }
    // is this url handled by a currently loaded plugin?
    else if(ResolverService * rs = app()->resolver()->get_url_handler(url)) 
    {
        serve_body(rs->http_handler(url, parts, getvars, postvars, m_pauth),
                   req, rep );
    }
    else // unhandled request
    {
        rep = moost::http::reply::stock_reply(moost::http::reply::not_found);
        //rep.content = "make a valid request, kthxbye";
    }
    
}

//
// REST API that returns JSON
//

void 
playdar_request_handler::handle_rest_api(   map<string,string> qs, 
                                            const moost::http::request& req,
                                            moost::http::reply& rep,
                                            string permissions)
{
    using namespace json_spirit;
    
    ostringstream response; 
    do
    {
        if(qs["method"] == "stat") {
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
        
        if(qs["method"] == "resolve")
        {
            string artist = qs["artist"];
            string album  = qs["album"];
            string track  = qs["track"];
            // create a new query and start resolving it:
            boost::shared_ptr<ResolverQuery> rq(new ResolverQuery(artist, album, track));
            if(qs.count("mode")) rq->set_mode(qs["mode"]);
            // was a QID specified? if so, use it:
            if(qs.count("qid"))
            {
                if(!app()->resolver()->query_exists(qs["qid"]))
                {
                    rq->set_id(qs["qid"]);
                } else {
                    cout << "WARNING - resolve request provided a QID, but that QID already exists as a running query. Assigning a new QID." << endl;
                    // new qid assigned automatically if we don't provide one.
                }
            }
            if(!rq->valid()) // usually caused by empty track name or something.
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
        else if(qs["method"]=="get_results" && qs.count("qid")==1)
        {
            Object r;
            Array qresults;
            vector< boost::shared_ptr<PlayableItem> > results = app()->resolver()->get_results(qs["qid"]);
            BOOST_FOREACH(boost::shared_ptr<PlayableItem> pip, results)
            {
                qresults.push_back( pip->get_json() );
            }   
            r.push_back( Pair("qid", qs["qid"]) );
            r.push_back( Pair("refresh_interval", 1000) ); //TODO move to ResolveQuery
            r.push_back( Pair("query", app()->resolver()->rq(qs["qid"])->get_json()) );
            r.push_back( Pair("results", qresults) );
            
            write_formatted( r, response );
        }
        else if(qs["method"] == "list_queries")
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
        else if(qs["method"] == "list_artists")
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
        else if(qs["method"] == "list_artist_tracks")
        {
            Array qresults;
            artist_ptr artist = app()->library()->load_artist( qs["artistname"] );
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
    if(qs.count("jsonp")==1) // wrap in js callback
    {
        retval = qs["jsonp"];
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
    rep.headers[1].value = (qs.count("jsonp")==1)?"text/javascript":"text/plain";
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
playdar_request_handler::serve_body(string reply, const moost::http::request& req, moost::http::reply& rep)
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
    serve_body(reply.str(), req, rep);
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
playdar_request_handler::serve_sid(const moost::http::request& req, moost::http::reply& rep, source_uid sid)
{
    cout << "Serving SID " << sid << endl;
    boost::shared_ptr<PlayableItem> pip = app()->resolver()->get_pi(sid);
    cout << "-> PlayableItem: " << pip->artist() << " - " << pip->track() << endl;
    boost::shared_ptr<StreamingStrategy> ss = pip->streaming_strategy();
    cout << "-> " << ss->debug() << endl;
    // hand off the streaming strategy for the http server to do:
    rep.set_streaming(ss, pip->size());
    return;  
}


// serves a .html file from docroot, but string substitutes stuff in the vars map.
void
playdar_request_handler::serve_dynamic( const moost::http::request& req, moost::http::reply& rep,
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
playdar_request_handler::serve_track(const moost::http::request& req, moost::http::reply& rep, int tid)
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

