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
#if __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

/** returns false if Audioscrobbler is not installed */
bool scrobsub_launch_audioscrobbler()
{
#if __APPLE__
    FSRef fsref;
    OSStatus err = LSFindApplicationForInfo(kLSUnknownCreator, CFSTR("fm.last.Audioscrobbler"), NULL, &fsref, NULL);
    if (err == kLSApplicationNotFoundErr) 
        return false;
    
    LSApplicationParameters p = {0};
    p.flags = kLSLaunchDontSwitch | kLSLaunchAsync;
    p.application = &fsref;
    LSOpenApplication( &p, NULL ); //won't launch if already running
    return true; //TODO if failed to launch we should log it
#else
    return false;
#endif
}

#if __APPLE__
static void script(const char* cmd)
{
    char a[] = "osascript -e 'tell application \"Audioscrobbler\" to ";
    char b[sizeof(a)+strlen(cmd)+2];
    strcpy(b, a);
    strcat(b, cmd);
    strcat(b, "'");
    system(b);
}

void scrobsub_relay(int state)
{
    switch(state){
        case SCROBSUB_PLAYING: script("resume \""SCROBSUB_CLIENT_ID"\""); break;
        case SCROBSUB_PAUSED:  script("pause \""SCROBSUB_CLIENT_ID"\""); break;
        case SCROBSUB_STOPPED: script("stop \""SCROBSUB_CLIENT_ID"\""); break;
    }
}

void scrobsub_relay_start(const char* artist, const char* title, int duration)
{
    #define FORMAT "start \""SCROBSUB_CLIENT_ID"\" with \"%s\" by \"%s\" duration %d"
    char s[sizeof(FORMAT)+strlen(artist)+strlen(title)];
    snprintf(s, sizeof(s), FORMAT, title, artist, duration);
    script(s);
}

#else
void scrobsub_relay(int state)
{}

void scrosub_relay_start(const char* artist, const char* title, int duration)
{}
#endif //__APPLE__
