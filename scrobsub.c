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

// Created by Max Howell <max@last.fm>

#include "scrobsub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static time_t start_time = 0;
static time_t pause_time = 0;
static int state = SCROBSUB_STOPPED;
static char* session_id = 0;
static unsigned int N = 0;
static char* np_url = 0;
static char* submit_url = 0;

static char* artist;
static char* track;
static char* album;
static char* mbid;
static unsigned int duration;
static unsigned int track_number;

void(*scrobsub_callback)(int event, const char* message);
void scrobsub_get(char* response, const char* url);
void scrobsub_post(char* response, const char* url, const char* post_data);
bool scrobsub_retrieve_credentials();
bool scrobsub_launch_audioscrobbler();

#if SCROBSUB_NO_RELAY
// compiler will optimise this stuff away now
#define relay false
#define scrobsub_relay(x)
#define scrobsub_relay_start(x, y, z);
#else
static bool relay = true;
void scrobsub_relay(int);
void scrobsub_relay_start(const char*, const char*, int);
#endif

char* scrobsub_session_key = 0;
char* scrobsub_username = 0;


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

#if !SCROBSUB_NO_RELAY
    // will return true if audioscrobbler is installed
    relay = scrobsub_launch_audioscrobbler();
#endif
    if(!relay && !scrobsub_retrieve_credentials())
        (callback)(SCROBSUB_AUTH_REQUIRED, 0);
}    

static void get_handshake_auth(char out[33], time_t time)
{
    char auth[32+10+1];
    strcpy(auth, SCROBSUB_SHARED_SECRET);
    snprintf(&auth[32], 11, "%d", time);
    scrobsub_md5(out, auth);
}

static char* handshake_response_strdup(char** p)
{
    char* start = *p;
    while (*++(*p)) if (**p == '\n') break;
    *(*p)++ = '\0';
    return strdup(start);
}

static bool ok(char* response)
{
    return response[0] == 'O' && response[1] == 'K' && response[2] == '\n';
}

static void handshake()
{
    scrobsub_finish_auth();
    
    if (!scrobsub_session_key || !scrobsub_username)
        return; //TODO auth required
    
    time_t time = now();
    char auth[33];
    get_handshake_auth(auth, time);
    int n = 34+8+8+6+11+3+strlen(scrobsub_username)+13+32+9+32+4+32+1;
    char url[n];
    
    n = snprintf(url, n, "http://post.audioscrobbler.com:80/"
                 "?hs=true"
                 "&p=1.2.1"
                 "&c=" SCROBSUB_CLIENT_ID      // length 3
                 "&v=" SCROBSUB_CLIENT_VERSION // length 8 max
                 "&u=%s"
                 "&t=%d" // length 10 for the next millenia at least :P
                 "&a=%s" // length 32
                 "&api_key=" SCROBSUB_API_KEY // length 32
                 "&sk=%s", // length 32
                 scrobsub_username, time, auth, scrobsub_session_key);
    if (n<0) return; //TODO error callback

    char responses[256];
    char* response = responses;
    scrobsub_get(responses, url);

    if(ok(response)){
        response += 3;
        session_id = handshake_response_strdup(&response);
        np_url = handshake_response_strdup(&response);
        submit_url = handshake_response_strdup(&response);
    }else
        //TODO better
        (scrobsub_callback)(SCROBSUB_ERROR_RESPONSE, response);
}

static unsigned int scrobble_time(unsigned int duration)
{
    if(duration>240*2) return 240;
    if(duration<30*2) return 30;
    return duration/2;
}

static void submit()
{    
    //TODO check track is valid to submit
    
    if(state == SCROBSUB_PAUSED)
        scrobsub_resume(); //sets pause time correctly
    //FIXME the second resolution issue can introduce rounding errors
    if(now() - (start_time + pause_time) < scrobble_time(duration))
        return;
    
    int n = 32+N+4+2+10+1 +2+9*6;
    char post_data[n];
    
    n = snprintf(post_data, n,
                     "s=%s"
                 "&a[0]=%s"
                 "&t[0]=%s"
                 "&b[0]=%s"
                 "&l[0]=%d"
                 "&n[0]=%d"
                 "&m[0]=%s"
                 "&i[0]=%d"
                 "&o[0]=%c"
                 "&r[0]=",
                 session_id, artist, track, album, duration, track_number, mbid, start_time, 'P');
    
    for (int x=0; x<2; ++x){    
        char response[128];
        scrobsub_post(response, submit_url, post_data);
        
        if(ok(response)) break;
        printf("E: %s: %s%s", __FUNCTION__, response, post_data);
        if(strcmp(response, "BADSESSION\n") != 0) break;
        handshake();
    }
}

void scrobsub_start(const char* _artist, const char* _track, unsigned int _duration, const char* _album, unsigned int _track_number, const char* _mbid)
{
    state = SCROBSUB_PLAYING;
    
    if(_duration>9999) _duration = 9999;
    if(_track_number>99) _track_number = 99;
    
    if(relay){
        scrobsub_relay_start(_artist, _track, _duration);
        return;
    }
    
    if (!session_id)
        handshake();
    if (state != SCROBSUB_STOPPED)
        submit();
    
    artist = strdup(_artist);
    track = strdup(_track);
    album = strdup(_album);
    mbid = strdup(_mbid);
    duration = _duration;
    track_number = _track_number;

    start_time = now();
    
    N = strlen(_artist)+strlen(_track)+strlen(_album)+strlen(_mbid);
    
    //TODO, don't emit np if user is skipping fast, then you need a timer
    //    static time_t previous_np = 0;
    //    time_t time = now();
    //    if(time - previous_np < 4)
    
    //TODO don't submit track number if 0

    char post_data[32+4+2+N+2+6*3];
    snprintf(post_data, sizeof(post_data),
                           "s=%s"
                          "&a=%s"
                          "&t=%s"
                          "&b=%s"
                          "&l=%d"
                          "&n=%d"
                          "&m=%s",
                          session_id,
                          artist,
                          track,
                          album,
                          duration,
                          track_number,
                          mbid);
    
    for(int x = 0; x < 2; ++x){
        char response[256];
        scrobsub_post(response, np_url, post_data);
        if(ok(response))
            break;
        handshake();
    }
}

void scrobsub_pause()
{
    if(relay)
        scrobsub_relay(SCROBSUB_PAUSED);
    else if(state == SCROBSUB_PLAYING){
        state = SCROBSUB_PAUSED;
        // we subtract pause_time so we continue to keep a record of the amount
        // of time paused so far
        pause_time = now() - pause_time;
    }
}

void scrobsub_resume()
{
    if(relay)
        scrobsub_relay(SCROBSUB_PLAYING);
    else if(state == SCROBSUB_PAUSED){
        pause_time = now() - pause_time;
        state = SCROBSUB_PLAYING;
    }
}

void scrobsub_stop()
{
    if(relay)
        scrobsub_relay(SCROBSUB_STOPPED);
    else if(state != SCROBSUB_STOPPED){
        submit();
        state = SCROBSUB_STOPPED;
        free( artist ); free( track ); free( album ); free( mbid );
        artist = track = album = mbid = 0;
        duration = track_number = 0;
    }
}


int scrobsub_state()
{
    return state;
}
