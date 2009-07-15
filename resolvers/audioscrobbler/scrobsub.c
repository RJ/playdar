/*
   Copyright 2009 Last.fm Ltd.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   
   This file was originally created by Max Howell <max@last.fm>
*/
#include "scrobsub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <malloc.h>
#define snprintf _snprintf
#endif

static time_t start_time = 0;
static time_t pause_time = 0;
static int state = SCROBSUB_STOPPED;
static char* session_id = 0;
static unsigned int N = 0; // cached length of artist+track+album+mbid
static char* np_url = 0;
static char* submit_url = 0;
static char rating = ' ';

static char* artist = 0;
static char* track = 0;
static char* album = 0;
static char* mbid = 0;
static unsigned int duration = 0;
static unsigned int track_number = 0;

void(*scrobsub_callback)(int event, const char* message);
void scrobsub_get(char* response, const char* url);
void scrobsub_post(char* response, const char* url, const char* post_data);
bool scrobsub_retrieve_credentials();
bool scrobsub_launch_audioscrobbler();

#if SCROBSUB_NO_RELAY
// compiler will optimise this stuff away now (in theory)
#define relay false
#define scrobsub_relay(x)
#define scrobsub_relay_start(x, y, z)
#else
static bool relay = true;
void scrobsub_relay(int);
void scrobsub_relay_start(const char*, const char*, int);
#endif

char* scrobsub_session_key = 0;
char* scrobsub_username = 0;


/** worth noting that this is UTC and not local */
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
    
    artist = (char*)malloc(0); // these must always point at malloc'd data
    track = (char*)malloc(0);
    album = (char*)malloc(0);
    mbid = (char*)malloc(0);
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

static char* escape(const char* in)
{
    // code from QByteArray.cpp, Qt 4.4.0
    
    const char hexnumbers[] = "0123456789ABCDEF"; 
    #define toHexHelper(c) hexnumbers[(c) & 0xf]
    
    int const n = strlen(in); 
    int const nn = n*3 + 1;
#if SCROBSUB_NO_C99
    // we only use alloca on Windows as its use is discouraged on BSD
    char* outs = (char*)alloca(nn);
#else
    char outs[nn];
#endif
    
    char* out = outs;
    for(int i = 0; i < n; ++i){
        char c = *in++;
        if(c >= 0x61 && c <= 0x7A // ALPHA
           || c >= 0x41 && c <= 0x5A // ALPHA
           || c >= 0x30 && c <= 0x39 // DIGIT
           || c == 0x2D // -
           || c == 0x2E // .
           || c == 0x5F // _
           || c == 0x7E)// ~
            *out++ = c;
        else{
            *out++ = '%';
            *out++ = toHexHelper((c & 0xf0) >> 4);
            *out++ = toHexHelper(c & 0xf);
        }
    }
    *out = '\0';

    return strdup(outs);
}

static bool handshake()
{
    scrobsub_finish_auth();
    
    if (!scrobsub_session_key || !scrobsub_username)
        return false; //TODO auth required AGAIN
    
    char* username = escape(scrobsub_username);
    time_t time = now();
    char auth[33];
    get_handshake_auth(auth, time);
    int n = 34+8+8+6+11+3+strlen(username)+13+32+9+32+4+32+1;
#if SCROBSUB_NO_C99
    char* url = (char*)alloca(n); //alloca discouraged on BSD
#else
    char url[n];
#endif
    
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
                 username, time, auth, scrobsub_session_key);
    free(username);

    if (n<0) return false; //TODO error callback

    char responses[256];
    scrobsub_get(responses, url);
    
    char* response = responses;
    if(ok(response)){
        response += 3;
        session_id = handshake_response_strdup(&response);
        np_url = handshake_response_strdup(&response);
        submit_url = handshake_response_strdup(&response);
        return true;
    }else{
        //TODO better
        (scrobsub_callback)(SCROBSUB_ERROR_RESPONSE, response);
        return false;
    }
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
    if(now()-(start_time+pause_time) < scrobble_time(duration))
        return;
    
    int n = 32+N+4+2+10+1+1 +2+9*6;
#if SCROBSUB_NO_C99
    char* post_data = (char*)alloca(n); //alloca discouraged on BSD
#else
    char post_data[n];
#endif
    
    for(int x=0; x<2; ++x){
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
                     "&r[0]=%c",
                     session_id, artist, track, album, duration, track_number, mbid, start_time, 'P', rating);
        char response[128];
        scrobsub_post(response, submit_url, post_data);

        if(ok(response)) break;
        printf("E: %s: %s%s", __FUNCTION__, response, post_data);
        if(strcmp(response, "BADSESSION\n") != 0) break;
        handshake();
    }
}

void scrobsub_change_metadata(const char* _artist, const char* _track, const char* _album)
{
    free(artist);
    free(track);
    free(album);
    artist = escape(_artist);
    track = escape(_track);
    album = escape(_album);    
    N = strlen(artist)+strlen(track)+strlen(album)+strlen(mbid);
}

void scrobsub_start(const char* _artist, const char* _track, unsigned int _duration, const char* _album, unsigned int _track_number, const char* _mbid)
{
    if(_duration>9999) _duration = 9999;
    if(_track_number>99) _track_number = 99;
    
    if(relay){
        scrobsub_relay_start(_artist, _track, _duration);
        state = SCROBSUB_PLAYING;
        return;
    }

    if (!session_id && !handshake())
        return;
    if (state != SCROBSUB_STOPPED)
        submit();
    
    state = SCROBSUB_PLAYING;
    
    free(mbid);
    mbid = strdup(_mbid);
    duration = _duration;
    track_number = _track_number;
    rating = ' ';
    scrobsub_change_metadata(_artist, _track, _album);
    start_time = now();
    pause_time = 0;

    //TODO don't submit track number if 0

    #define POST_DATA_LENGTH 32+4+2+N+2+6*3
#if SCROBSUB_NO_C99
    char* post_data = (char*)alloca(POST_DATA_LENGTH); //alloca discouraged on BSD
#else
    char post_data[POST_DATA_LENGTH];
#endif    
    for(int x = 0; x < 2; ++x){
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
        duration = track_number = 0;
        // indeed don't free or set artist, track etc. to NULL, they must always
        // point to somthing malloc'd. We free in scrobsub_start()
    }
}

int scrobsub_state()
{
    return state;
}

void scrobsub_love()
{
    rating='L';
}

void scrobsub_rate( char c ) //undocumented ;)
{
    rating=c;
}
