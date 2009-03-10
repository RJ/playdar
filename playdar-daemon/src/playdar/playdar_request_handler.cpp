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

#include "playdar_request_handler.h"
#include "playdar/library.h"
#include "playdar/resolver.h"

void 
playdar_request_handler::init(MyApplication * app)
{
    cout << "HTTP handler online." << endl;

    m_app = app;
}

void 
playdar_request_handler::handle_request(const moost::http::request& req, moost::http::reply& rep)
{
    cout << "HTTP GET " << req.uri << endl;
    rep.unset_streaming();
    UriParserStateA state;
    UriQueryListA * queryList;
    UriUriA uri;
    state.uri = &uri;
    int qsnum; // how many pairs in querystring
    // basic uri parsing
    if (uriParseUriA(&state, req.uri.c_str()) != URI_SUCCESS)
    {
        cerr << "FAILED TO PARSE URI" << endl;
        rep = moost::http::reply::stock_reply(moost::http::reply::bad_request );
        return;
    }
    
    // querystring parsing
    map<string,string> querystring;
    if ( uriDissectQueryMallocA(&queryList, &qsnum, uri.query.first, 
            uri.query.afterLast) != URI_SUCCESS)
    {
        // this just means the URL doesnt have a querystring
        uriFreeUriMembersA(&uri);
    } else
    {
        UriQueryListA * q = queryList;
        for(int j=0; j<qsnum; j++)
        {
            querystring[q->key]=q->value;
            if(q->next) q = q->next;
        }
        if(queryList) uriFreeQueryListA(queryList);
        uriFreeUriMembersA(&uri);
    }

    // get first /dir/ bit
    vector<std::string> parts;
    boost::split(parts, req.uri, boost::is_any_of("/"));
    
    if(parts[1] == "") // Req: /
    {
        serve_body("<h1>Playdar: " + app()->conf()->get<string>("name")
         + "</h1><a href='/stats'>Stats</a><hr/>For quick and dirty resolving, you can try constructing an URL like: <code>"+app()->conf()->httpbase()+"/quickplay/ARTIST/ALBUM/TRACK</code><br/><br/>For the real demo that uses the JSON API, check <a href=\"http://www.playdar.org/\">Playdar.org</a>.", req, rep);
    }
    else if(parts[1]=="auth_1")
    {
        map<string,string> vars;
        string filename = "/home/rj/src/playdar/playdar-daemon/www/static/auth.html";
        string formtoken = "my_form_token_9827342";
        string url = querystring["receiverurl"];
        vars["<%URL%>"]=url;
        vars["<%FORMTOKEN%>"]=formtoken;
        serve_dynamic(req, rep, filename, vars);
    }
    else if(parts[1]=="auth_2")
    {
        map<string,string> vars;
        string filename = "/home/rj/src/playdar/playdar-daemon/www/static/auth.html2";
        string authtoken = "auth_token_123";
        string url = querystring["receiverurl"];
        url += "#";
        url += authtoken;
        vars["<%URL%>"]=url;
        serve_dynamic(req, rep, filename, vars);
    }
    else if(parts[1]=="static") 
    {
        serve_static_file(req, rep);
    }
    else if(parts[1]=="streaming") 
    {
        boost::shared_ptr<StreamingStrategy> ss(new LocalFileStreamingStrategy("/home/rj/fakemp3/01-Train Train-Jag.mp3"));
        rep.set_streaming(ss, 1234);
        return;        
    }
    else if(parts[1]=="api" && querystring.count("method")==1)
    {
        handle_rest_api(querystring, req, rep);
    }
    else if(parts[1]=="stats") 
    {
        serve_stats(req, rep);
    }
    // quick hack method:
    else if(parts[1]=="quickplay" && parts.size() == 5) // Req: /resolve/artist/album/track 
    {
        
        string artist   = unescape(parts[2]);
        string album    = unescape(parts[3]);
        string track    = unescape(parts[4]);
        boost::shared_ptr<ResolverQuery> rq(new ResolverQuery(artist, album, track));
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
    else if(parts[1]=="serve" && parts.size() == 3)
    {
        int fid = atoi(parts[2].c_str());
        if(fid) serve_track(req, rep, fid);
    }
    else if(parts[1]=="sid" && parts.size() == 3)
    {
        source_uid sid = parts[2];
        serve_sid(req, rep, sid);
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
                                            moost::http::reply& rep)
{
    using namespace json_spirit;
    
    ostringstream response; 
    
    if(qs["method"] == "stat") {
        Object r;
        r.push_back( Pair("name", "playdar") );
        r.push_back( Pair("version", "0.1.0") );
        r.push_back( Pair("capabilities", "TODO") ); // might do something clever here later
        write_formatted( r, response );
    }
    else if(qs["method"] == "resolve")
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
    
    //cout << response.str() << endl;
    
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
    r << "<html><head><title>Playdar</title></head><body>\n";
//    r << "<div style=\"position:absolute; height: 30px; width:100%; background-color: #f0f0f0; padding:3px;\"><a href=\"/\">Playdar Index</a></div>\n";
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
    reply   << "<h1>Stats: " << app()->conf()->get<string>("name") 
            << "</h1>"
            << "<h2>Local Library</h2>"
            << "<table>"
            << "<tr><td>Num Files</td><td>" << app()->library()->num_files() << "</td></tr>\n"
            << "<tr><td>Artists</td><td>" << app()->library()->num_artists() << "</td></tr>\n"
            << "<tr><td>Albums</td><td>" << app()->library()->num_albums() << "</td></tr>\n"
            << "<tr><td>Tracks</td><td>" << app()->library()->num_tracks() << "</td></tr>\n"
            << "</table>"
            << "<h2>Resolver Stats</h2>"
            << "<table>"
            << "<tr><td>Num queries seen</td><td>" 
            << app()->resolver()->num_seen_queries() 
            << "</td></tr>\n"
            << "</table>"
            ;
    serve_body(reply.str(), req, rep);
}

void
playdar_request_handler::serve_static_file(const moost::http::request& req, moost::http::reply& rep)
{
    moost::http::filesystem_request_handler frh;
    frh.doc_root("/home/rj/src/playdar/playdar-daemon/www");
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
        string newline = line;
        BOOST_FOREACH(pair_t item, vars)
        {
            newline = boost::replace_all_copy(newline, item.first, item.second);
        }
        os << newline << endl;
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

