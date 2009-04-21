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

#ifndef __SCROBSUB_H__
#define __SCROBSUB_H__

#include <stdbool.h>

#if __cplusplus
extern "C" {
#endif

/** the callback must be set, when the callback is called, you get one of the
  * SCROBSUB_ values. */
void scrobsub_init(void(*callback)(int event, const char* message));

/** you need to call scrobsub_auth, but you can do it whenever you want,
  * although, no scrobbling will happen until then */
#define SCROBSUB_AUTH_REQUIRED 0
/** the char* paramater will be the error string */
#define SCROBSUB_ERROR_RESPONSE 1


/** the user needs to visit @p url within one hour for authentication to succeed */
void scrobsub_auth(char url[110]);

/** once the user has visited the above page, we need to request their session
  * key, you can either do this yourself, or scrobsub will do it as required,
  * mostly you won't need to call this function
  * @returns true if authentication was successful */
bool scrobsub_finish_auth();


/** A new track started. scrobsub takes copies of the strings. All strings must
  * be UTF8. 
  * artist, track and duration are mandatory
  * album and mbid can be "" (do not pass NULL or 0)
  * if track_number is 0 it is ignored (yes, we know some albums have a zeroth track)
  * valid MBIDs are always 38 characters, be wary of that
  */
void scrobsub_start(const char* artist, const char* track, unsigned int duration, const char* album, unsigned int track_number, const char* mbid);
/** the thing that we're scrobbling got paused. This is not a toggle, when/if
  * the track is unpaused, call resume. We insist on this distinction because
  * we want you to be exact! */
void scrobsub_pause();
/** the thing that we're scrobbling was unpaused */
void scrobsub_resume();
/** only call this when playback stops, if a new track is about to start, call
  * scrobsub_start() instead */
void scrobsub_stop();

/** marks the current track as loved, it is worth noting, you also have to call
  * the Last.fm track.love webservice separately (scrobsub doesn't do it for 
  * you). This stupid system will be like this forever prolly. Sorry about that.
  */
void scrobsub_love();


#define SCROBSUB_STOPPED 0
#define SCROBSUB_PLAYING 1
#define SCROBSUB_PAUSED 2

int scrobsub_state();


/** returns 0 if you need to auth, or the user still hasn't allowed the auth 
  * attempt */
extern char* scrobsub_session_key;
extern char* scrobsub_username;


/** for your convenience, we need it, so maybe you can use it too */
void scrobsub_md5(char out[33], const char* in);


#if __cplusplus
}
#endif
#endif
