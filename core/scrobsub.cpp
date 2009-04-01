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

#define STOPPED 0
#define PLAYING 1
#define PAUSED 2

static void(*callback)(int event, char* message) = 0;
static bool enabled = true;
static time_t start_time = 0;
static time_t pause_time = 0;
static int state = STOPPED;
static const char* shared_secret = 0;
static char* session_id = 0;
static uint N = 0;


static bool get_session_key()
{
    return 0;
}

static bool use_the_moose()
{
    return true;
}



void scrobsub_init(void(*callbackp)(int event, char* message))
{
    callback = callbackp;
    
    if(!use_the_moose() && !get_session_key())
        (callback)(SCROBSUB_AUTH_REQUIRED, 0);
} 


void scrobsub_set_enabled(bool enabledp)
{
    enabled = enabledp;
}


void scrobsub_auth(char* api_key, char* shared_secret)
{
    //TODO
}


static void get_auth(time_t time, char out[33])
{
    char auth[32+10+1];
    strcpy(auth, shared_secret);
    snprintf(auth[32], 11, "%d", time);
    md5(out, auth);
}


static void handshake()
{
    char* username = get_username();
    time_t time = now();
    char auth[33];
    get_auth(auth, time);
    int const n = 8+8+6+11+3+strlen(username)+13+32+9+32+4+32+1;
    char query[n];

    n = snprintf(query, n, "?hs=true"
                           "&p=1.2.1"
                           "&c=" SCROBSUB_CLIENT_ID      // length 3
                           "&v=" SCROBSUB_CLIENT_VERSION // length 8 max
                           "&u=%s"
                           "&t=%d" // length 10 for the next millenia at least :P
                           "&a=%s" // length 32
                           "&api_key=" SCROBSUB_API_KEY // length 32
                           "&sk=%s", // length 32
                           username, time, auth, get_session_key());
    if (n<0) return;

    char* response = query;
    get(response, "post.audioscrobbler.com", 80, query);
}


void scrobsub_start(char* artist, char* track, char* album, char* mbid, uint duration, uint track_number)
{
    //TODO
//    static time_t previous_np = 0;
//    time_t time = now();
//    if(time - previous_np < 4)
    
    state = PLAYING;
    
    if(use_the_moose()){
        moose_push(artist, track, album, mbid, duration, track_number);
        return;
    }
        
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
        post(response, np_host, np_port, np_path, post_data);
        if(strcmp(response, "OK") == 0)
            break;
        handshake();
    }
}


void scrobsub_pause()
{
    if(state == PLAYING){
        state = PAUSED;
        // we subtract pause_time so we continue to keep a record of the amount
        // of time paused so far
        pause_time = now() - pause_time;
    }
}


void scrobsub_resume()
{
    if(state == PAUSED){
        pause_time = now() - pause_time;
        state = PLAYING
    }
}


static uint scrobble_time(uint duration)
{
    if(duration>240*2) return 240;
    if(duration<30) return 30;
    return duration/2;
}


static void submit()
{
    //TODO check track is valid to submit
    
    if(state == PAUSED)
        scrobsub_resume();
    if(now() - timestamp + pause_time < scrobble_time(duration))
        return;

    int n = 32+N+10+1 +2+9*5
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
        if(strcmp(response,"BADSESSION")
            handshake();
        break;
    }
}


void scrobsub_stop()
{
    if(state != STOPPED)
        submit();
    state = STOPPED;
}
