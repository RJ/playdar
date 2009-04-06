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

#ifndef __SCROBSUB_H__
#define __SCROBSUB_H__

#include <stdbool.h>

/** the callback must be set, when the callback is called, you get one of the
  * SCROBSUB_ values. */
void scrobsub_init(void(*callback)(int event, const char* message));

/** you need to call scrobsub_auth, but you can do it whenever you want, 
  * although, no scrobbling will happen until then */
#define SCROBSUB_AUTH_REQUIRED 0
/** the char* paramater will be the error string */
#define SCROBSUB_ERROR_RESPONSE 1


/** calling false will disable submission to Last.fm. However it will not
  * disable it if the official Last.fm Audioscrobbler client is running. This is
  * because the user disables scrobbling centrally there in that case. */
void scrobsub_set_enabled(bool enabled);


/** the user needs to visit @p url within one hour for authentication to succeed */
void scrobsub_auth(char url[110]);


/** A new track started. scrobsub takes copies of the strings. All strings must
  * be UTF8. */
void scrobsub_start(const char* artist, const char* track, const char* album, unsigned int duration, unsigned int track_number, const char* mbid);
void scrobsub_pause();
void scrobsub_resume();
/** only call this when playback stops, if a new track is about to start, call
  * scrobsub_start() instead */
void scrobsub_stop();


#define SCROBSUB_STOPPED 0
#define SCROBSUB_PLAYING 1
#define SCROBSUB_PAUSED 2

int scrobsub_state();


/** returns 0 if you need to auth, or the user still hasn't allowed the auth 
  * attempt */
const char* scrobsub_session_key();
const char* scrobsub_username();


/** for your convenience, we need it, so maybe you can use it too */
void scrobsub_md5(char out[33], const char* in);

#endif
