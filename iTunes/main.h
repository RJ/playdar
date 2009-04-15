/***************************************************************************
 *   Copyright 2004-2009 by                                                *
 *      Gareth Simpson <iscrobbler@xurble.org>                             *
 *      Last.fm Ltd.                                                       *
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

// Don't allow windows.h to include old winsock.h because it conflicts with
// winsock2.h included by the ScrobSubmitter
#ifndef _WINSOCKAPI_
    #define _WINSOCKAPI_
#endif

#include "VisualAPI/iTunesVisualAPI.h"
#include <string>


#if TARGET_OS_WIN32
    #define	MAIN iTunesPluginMain
    #define IMPEXP	__declspec(dllexport)
#else
    #define	MAIN iTunesPluginMainMachO
    #define IMPEXP
#endif


#define kVisualPluginName     "Last.fm AudioScrobbler"
#define kVisualPluginCreator  '\?\?\?\?'


// You must update the version in many places young one, see the README
#define kVisualPluginMajorVersion     3
#define kVisualPluginMinorVersion     0
#define kVisualPluginReleaseStage     1
#define kVisualPluginNonFinalRelease  0


struct VisualPluginData 
{
    void*               appCookie;
    ITAppProcPtr        appProc;
    
#if TARGET_OS_MAC
    ITFileSpec          pluginFileSpec;
#endif
    
    GRAPHICS_DEVICE     destPort;
    Rect                destRect;
    OptionBits          destOptions;
    UInt32              destBitDepth;
    
    RenderVisualData    renderData;
    UInt32              renderTimeStampID;
    
    SInt8               waveformData[kVisualMaxDataChannels][kVisualNumWaveformEntries];
    
    UInt8               level[kVisualMaxDataChannels];      /* 0-128 */
    
    ITTrackInfoV1       trackInfo;
    ITStreamInfoV1      streamInfo;
    
    Boolean             playing;
    Boolean             padding[3];
    
    /** Plugin-specific data */
#if TARGET_OS_MAC
    GWorldPtr           offscreen;
#endif
};

typedef struct VisualPluginData VisualPluginData;


#if TARGET_OS_WIN32	
    void ScrobSubCallback( int, 
                           bool, 
                           std::string, 
                           void* );

    void ClearMemory( LogicalAddress, SInt32 );

#else
    void notificationCallback( CFNotificationCenterRef,
                               void*,
                               CFStringRef,
                               const void*,
                               CFDictionaryRef );
#endif


std::string GetVersionString();

OSStatus VisualPluginHandler( OSType,
                              VisualPluginMessageInfo*,
                              void* );
