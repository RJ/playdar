/*
    Playdar - music content resolver
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
#include "audioscrobbler_plugin.h"
#include "scrobsub.h"
#include "playdar/playdar_request.h"
#include "playdar/playdar_response.h"
#include "playdar/logger.h"

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
        log::info() << "You need to authenticate scrobbling, visit http://localhost:60210/audioscrobbler/config/" << endl; //FIXME hardcoded url
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
    unsigned int duration = atoi(GET("l")); // zero on error, which scrobsub will reject, so all good
    
    // todo shouldn't return 0 for errors as 0 can be valid
    unsigned int track_number = atoi(GET("n"));

    scrobsub_start(artist, track, duration, album, track_number, mbid);
}

static string config(bool auth_required)
{
    if (scrobsub_query_relay())
        return "<p>Playdar is relaying scrobbles to your official Last.fm Audioscrobbler client.</p>";
    
    if (!auth_required || scrobsub_finish_auth() && scrobsub_username)
        return "<p>Playdar is authorized to "
               "<a href='http://www.last.fm/help/faq?category=Scrobbling'>scrobble</a> "
               "as "+string(scrobsub_username)+". "
               "To revoke, visit <a href='http://www.last.fm/settings/applications'>Last.fm</a>.</p>";
    
    char url[110];
    scrobsub_auth(url);
    return string("<p>You need to <a href='") + url + "'>authenticate</a> in order to scrobble.</p>";
}

bool 
audioscrobbler::anon_http_handler(const playdar_request& rq, playdar_response& resp, playdar::auth& /*pauth*/ )
{
    if( rq.parts()[1] == "config" )
    {
        resp = config(auth_required);
        return true;
    }
    return false;
}

bool
audioscrobbler::authed_http_handler(const playdar_request& rq, playdar_response& resp,  playdar::auth& pauth)
{
    log::info() << "audioscrobbler: Authed HTTP" << endl;
    
    if(rq.parts().size()<2) return false;

    string action = rq.parts()[1];

    std::string s1, s2;
    if(rq.getvar_exists("jsonp")){ // wrap in js callback
        s1 = rq.getvar("jsonp") + "(";
        s2 = ");\n";
    }
    // TODO, use json_spirit
    playdar_response ok( s1 + "{\"success\" : true, \"action\" : \"" + action + "\"}" + s2, false );
    
    if(action == "start")  { start(rq); resp = ok; return true;}
    if(action == "pause")  { scrobsub_pause(); resp = ok; return true;}
    if(action == "resume") { scrobsub_resume(); resp = ok; return true;}
    if(action == "stop")   { scrobsub_stop(); resp = ok; return true;}
    
    return false;
}

EXPORT_DYNAMIC_CLASS( audioscrobbler )
