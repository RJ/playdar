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

static void(*callback)(int event, char* message);
static bool enabled;
static time_t start_time;
static time_t pause_time;
static int state;
static const char* shared_secret;


#define STOPPED 0
#define PLAYING 1
#define PAUSED 2


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
    
    if(!use_the_moose() && !session_key())
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
                           "&c=" SCROBSUB_CLIENT_ID      // max 3 chars
                           "&v=" SCROBSUB_CLIENT_VERSION // max 8 chars
                           "&u=%s"
                           "&t=%d" // length 10 for the next 1000 years at least :P
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
    state = PLAYING
    
    if(use_the_moose()){
        moose_push(artist, track, album, mbid, duration, track_number);
        return;
    }
    
    
}


void scrobsub_pause()
{
    if(state != PLAYING)
        return;

    state = PAUSED;
    pause_time = now();
}


void scrobsub_resume()
{
    
}


void scrobsub_stop()
{
    
}


void scrobsub_force_submit()
{
    
}
