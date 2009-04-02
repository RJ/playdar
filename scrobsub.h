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

#ifndef __SCROBSUB_PLUGIN_H__
#define __SCROBSUB_PLUGIN_H__

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


/** this will auth your application with Last.fm via the users web browser 
  * any submissions in the mean time will be queued */
void scrobsub_auth();


/** A new track started.
  * You are responsible for the memory of these pointers, it must exist until 
  * the next scrobsub_start or scrobsub_stop */
void scrobsub_start(const char* artist, const char* track, const char* album, const char* mbid, unsigned int duration, unsigned int track_number);
void scrobsub_pause();
void scrobsub_resume();
/** only call this when playback stops, if a new track is about to start, call
  * scrobsub_start() instead */
void scrobsub_stop();


#define SCROBSUB_STOPPED 0
#define SCROBSUB_PLAYING 1
#define SCROBSUB_PAUSED 2

int scrobsub_state();


/** can be useful sometimes, has no effect if the official Auidioscrobbler is
  * calling the shots */
void scrobsub_force_submit();


/** returns true if there's a key, provide a char[33] as out if you want it, but 
  * feel free to pass 0 */ 
bool scrobsub_session_key(const char* out);


/** for your convenience, we use it, so maybe you need to as well */
void scrobsub_md5(char out[33], const char* in);

#endif
