#include "audioscrobbler_plugin.h"
#include "scrobsub.h"
#include "playdar/playdar_request.h"
#include "playdar/playdar_response.h"
using std::cout;
using std::endl;
using std::string;
using playdar::playdar_request;
using playdar::playdar_response;
using playdar::plugin::audioscrobbler;
static audioscrobbler* instance = 0;


void
audioscrobbler::scrobsub_callback(int e, const char*s)
{
    if (e == SCROBSUB_AUTH_REQUIRED && !instance->auth_required)
    {
        instance->auth_required = true;
        cout << "You need to authenticate scrobbling, visit http://localhost:8888/audioscrobbler/config/" << endl;
    }
}

bool
audioscrobbler::init(pa_ptr pap)
{
    if(instance)return false; //loading more than one scrobbling plugin is stupid
    instance = this;

    scrobsub_init(scrobsub_callback);
    return true;
}

void
audioscrobbler::Destroy()
{
    scrobsub_stop();
    instance = 0;
}

static void start(const playdar_request& rq)
{
    #define GET(x) rq.getvar_exists(x) ? rq.getvar(x).c_str() : ""
    
    const char* artist = GET("a");
    const char* track = GET("t");
    const char* source = GET("o");
    const char* album = GET("b");
    const char* mbid = GET("m");
    uint duration = atoi(GET("l")); // zero on error, which scrobsub will reject, so all good
    
    // todo shouldn't return 0 for errors as 0 can be valid
    uint track_number = atoi(GET("n"));

    scrobsub_start(artist, track, duration, album, track_number, mbid);
}

static string config(bool auth_required)
{
    if (!auth_required || scrobsub_finish_auth())
        return "<p>Playdar is authorized to <a href='http://www.last.fm/help/faq?category=Scrobbling'>scrobble</a>. "
               "To revoke, visit <a href='http://www.last.fm/settings/applications'>Last.fm</a>.</p>";
    
    char url[110];
    scrobsub_auth(url);
    return string("<p>You need to <a href='") + url + "'>authenticate</a> in order to scrobble.</p>";
}

playdar_response 
audioscrobbler::authed_http_handler(const playdar_request* rq, playdar::auth* pauth)
{
    if(rq->parts().size()<2) return "Hi index!";
    string action = rq->parts()[1];

    std::string s1, s2;
    if(rq->getvar_exists("jsonp")){ // wrap in js callback
        s1 = rq->getvar("jsonp") + "(";
        s2 = ");\n";
    }
    // TODO, use json_spirit
    playdar_response ok( s1 + "{\"success\" : true, \"action\" : \"" + action + "\"}" + s2, false );
    
    if(action == "start")  { start(*rq); return ok; }
    if(action == "pause")  { scrobsub_pause(); return ok; }
    if(action == "resume") { scrobsub_resume(); return ok; }
    if(action == "stop")   { scrobsub_stop(); return ok; }
    
    if(action == "config") return config(auth_required);
    
    return "Unhandled"; // --warning

}


EXPORT_DYNAMIC_CLASS( audioscrobbler )
