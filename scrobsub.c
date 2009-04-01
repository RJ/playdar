/***************************************************************************
 *   Copyright 2005-2009 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "scrobsub.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static bool enabled = true;
static time_t start_time = 0;
static time_t pause_time = 0;
static int state = SCROBSUB_STOPPED;
static char* session_id = 0;
static unsigned int N = 0;


void(*scrobsub_callback)(int event, const char* message);
void scrobsub_get(char* response, const char* host, const char* path);
void scrobsub_post(char* response, const char* host, const char* path, const char* post_data);

bool scrobsub_session_key(char* out)
{
    return false;
}


static bool use_the_moose()
{
#if SCROBSUB_NO_RELAY    
    // we are the moose
    return false;
#else
    return true;
#endif
}

static time_t now()
{
    time_t t;
    time(&t);
    mktime(gmtime(&t));
    return t;
}


void scrobsub_init(void(*callback)(int, const char*))
{
    scrobsub_callback = callback;
    
    if(!use_the_moose() && !scrobsub_session_key(NULL))
        (callback)(SCROBSUB_AUTH_REQUIRED, 0);
} 


void scrobsub_set_enabled(bool enabledp)
{
    enabled = enabledp;
}


static void get_auth(char out[33], time_t time)
{
    char auth[32+10+1];
    strcpy(auth, SCROBSUB_SHARED_SECRET);
    snprintf(&auth[32], 11, "%d", time);
    scrobsub_md5(out, auth);
}


static const char* get_username()
{
    return "mxcl"; //TODO
}


static void handshake()
{
    const char* username = get_username();
    time_t time = now();
    char auth[33];
    get_auth(auth, time);
    int n = 8+8+6+11+3+strlen(username)+13+32+9+32+4+32+1;
    char query[n];
    char session_key[33];
    scrobsub_session_key(session_key);

    n = snprintf(query, n, "?hs=true"
                           "&p=1.2.1"
                           "&c=" SCROBSUB_CLIENT_ID      // length 3
                           "&v=" SCROBSUB_CLIENT_VERSION // length 8 max
                           "&u=%s"
                           "&t=%d" // length 10 for the next millenia at least :P
                           "&a=%s" // length 32
                           "&api_key=" SCROBSUB_API_KEY // length 32
                           "&sk=%s", // length 32
                           username, time, auth, session_key);
    if (n<0) return;

    char* response = query;
    scrobsub_get(response, "post.audioscrobbler.com:80", query);
}


void scrobsub_start(const char* artist, const char* track, const char* album, const char* mbid, unsigned int duration, unsigned int track_number)
{
    state = SCROBSUB_PLAYING;

#if !SCROBSUB_NO_RELAY
    if(use_the_moose()){
        moose_push(artist, track, album, mbid, duration, track_number);
        return;
    }
#endif
    
    start_time = now();

    //TODO
    //    static time_t previous_np = 0;
    //    time_t time = now();
    //    if(time - previous_np < 4)   
    
    if (duration>9999) duration = 9999;
    if (track_number>99) track_number = 99;
    
    N = strlen(artist)+strlen(track)+strlen(album)+strlen(mbid);
    int n = 32+4+2+N +2+6*3;
    char post_data[n];
    snprintf(post_data, n, "s=%s"
                          "&a=%s"
                          "&t=%s"
                          "&b=%s"
                          "&l=%d"
                          "&n=%d"
                          "&m=%s",
                          session_id,
                          artist,
                          track,
                          duration,
                          track_number,
                          mbid);
    
    for(int x = 0; x < 2; ++x){
        char response[3];
        scrobsub_post(response, np_host, np_port, np_path, post_data);
        if(strcmp(response, "OK") == 0)
            break;
        handshake();
    }
}


void scrobsub_pause()
{
    if(state == SCROBSUB_PLAYING){
        state = SCROBSUB_PAUSED;
        // we subtract pause_time so we continue to keep a record of the amount
        // of time paused so far
        pause_time = now() - pause_time;
    }
}


void scrobsub_resume()
{
    if(state == SCROBSUB_PAUSED){
        pause_time = now() - pause_time;
        state = SCROBSUB_PLAYING;
    }
}


static unsigned int scrobble_time(unsigned int duration)
{
    if(duration>240*2) return 240;
    if(duration<30) return 30;
    return duration/2;
}


static void submit()
{
    //TODO check track is valid to submit
    
    if(state == SCROBSUB_PAUSED)
        scrobsub_resume();
    if(now() - timestamp + pause_time < scrobble_time(duration))
        return;

    int n = 32+N+10+1 +2+9*5;
    char post_data[n];
        
    n = snprintf(post_data, n, "s=%s"
                           "&a[0]=%s"
                           "&t[0]=%s"
                           "&b[0]=%s"
                           "&l[0]=%d"
                           "&n[0]=%d"
                           "&m[0]=%s"
                           "&i[0]=%d"
                           "&o[0]=%c"
                           "&r[0]=",
                           session_id, artist, track, album, duration, track_number, mbid, timestamp, 'P' );
        
    for (int x=0; x<2; ++x){    
        char response[128];
        post(response, submit_host, submit_port, submit_path, post_data);
        if(strcmp(response,"BADSESSION") == 0)
            handshake();
        break;
    }
}


void scrobsub_stop()
{
    if(state != SCROBSUB_STOPPED)
        submit();
    state = SCROBSUB_STOPPED;
}


int scrobsub_state()
{
    return state;
}
