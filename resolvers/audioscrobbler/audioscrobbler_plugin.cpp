#include "audioscrobbler_plugin.h"
#include "scrobsub.h"

using playdar::resolvers::audioscrobbler;


static void scrobsub_callback(int, const char*);


bool
audioscrobbler::init(playdar::Config* c, Resolver* r)
{
    ResolverService::init(c, r);
    scrobsub_init(scrobsub_callback);
}

void
audioscrobbler::Destroy()
{
    scrobsub_stop();
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
    uint track_number = atoi(GET("n"));

    scrobsub_start(artist, track, album, duration, track_number, mbid);
}

string
audioscrobbler::http_handler(const playdar_request& rq, playdar::auth* pauth)
{
    if(rq.parts().count()<2) return;
    string& action = rq.parts()[1];
    if(action == "start")  { start(rq); return; }
    if(action == "pause")  { scrobsub_pause(); return; }
    if(action == "resume") { scrobsub_resume(); return; }
    if(action == "stop")   { scrobsub_stop(); return; }
}
