/***************************************************************************
 *   Copyright 2008-2009 Last.fm Ltd.                                      *
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

#include "main.h"
#include "common/logger.h"
#include "ITunesPlaysDatabase.h"

/** @author <max@last.fm> */


enum PlayerState
{
    /** transitions: playback started, track changed, or track unpaused
      * sync: always */
      Playing,
    /** transitions: playback ended
      * sync: always
      * notes: mostly this only occurs after the last track in a playlist 
      *        finishes. However, it is possible for the iTunes pause button, 
      *        to become a stop button, ie. HTTP streaming. Basically this event
      *        is rarely seen */
      Stopped,
    /** transitions: track paused
      * sync: never */
      Paused,
      Unknown
};

static const char* kPlayerStates[4] = { "Playing", "Stopped", "Paused", "Unknown" };


static inline int state( CFDictionaryRef d )
{
    CFStringRef s = (CFStringRef) CFDictionaryGetValue( d, CFSTR( "Player State" ) );

    #define RETURN_IF_EQUAL( a, b ) \
            if (CFStringCompare( a, CFSTR( #b ), 0 ) == kCFCompareEqualTo ) \
                return b

    // in order of frequency
    RETURN_IF_EQUAL( s, Playing );
    RETURN_IF_EQUAL( s, Paused );
    RETURN_IF_EQUAL( s, Stopped );

    return Unknown;
}


void
notificationCallback( CFNotificationCenterRef,
                      void*,
                      CFStringRef,
                      const void*,
                      CFDictionaryRef info )
{
    int const newState = state( info );

    LOG( 3, "iTunes player-state became: " << kPlayerStates[newState] );

    switch (newState)
    {
        case Stopped:
        case Playing:
            ITunesPlaysDatabase::sync();
            break;
        default:
            break;
    }
}


OSStatus
VisualPluginHandler( OSType m, VisualPluginMessageInfo*, void* )
{
    switch (m)
    {
        case kVisualPluginInitMessage:
        case kVisualPluginCleanupMessage:
            // or we don't get the cleanup message in main()
//            return noErr; TODO
        default:
            return unimpErr;
    }
}
