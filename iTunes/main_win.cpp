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

#include "main.h"

#include "common/c++/logger.h"
#include "ITunesComThread.h"
#include "Moose.h"
#include "ScrobSubmitter.h"

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cwctype>
#include <cstdlib>


// WTL things
HANDLE gHModule; // WTL handle populated in DLLMain

extern ScrobSubmitter gSubmitter;
extern ITunesComThread* gCom;

// reporting things
std::wstring gStatusStringX; // just some temp debug info (even for the release build) 
std::wstring gErrStringX; // an error

// the current track info
std::wstring gArtist;
std::wstring gTrack;
std::wstring gAlbum;
std::wstring gPath;
std::wstring gKind;
int gLen = 0;
int gStars = 0;
int gBitrate = 0;

DWORD gTimeOfLastStop = 0;


enum EAsState
{
    AS_STOPPED,
    AS_PLAYING,
    AS_PAUSED
};

EAsState gASState = AS_STOPPED;


/******************************************************************************
 * Utility functions                                                          *
 ******************************************************************************/

void
SetError( const std::wstring& err )
{
    LOGWL( 1, err );

    gErrStringX = err;
}


void
SetStatus( const std::wstring& stat )
{
    LOGWL( 3, stat );

    gStatusStringX = stat;
}


void
ClearMemory( LogicalAddress dest, SInt32 length )
{
    register unsigned char *ptr;

    ptr = (unsigned char *) dest;

    if( length > 16 )
    {
        register unsigned long  *longPtr;

        while( ((unsigned long) ptr & 3) != 0 )
        {
            *ptr++ = 0;
            --length;
        }

        longPtr = (unsigned long *) ptr;

        while( length >= 4 )
        {
            *longPtr++  = 0;
            length      -= 4;
        }

        ptr = (unsigned char *) longPtr;
    }

    while( --length >= 0 )
    {
        *ptr++ = 0;
    }
}


static bool
TryMemory( wchar_t* ptr )
{
    wchar_t character;
    __try
    {
        character = *ptr;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        return false;
    }
    return true;
}


static wchar_t*
SearchForPathDirectional( wchar_t*      pStart,
                          std::wstring& fileName,
                          int           increment,
                          int           bytesToSearch )
{
    int count = increment;

    while ( abs( count ) < bytesToSearch )
    {
        // We have to check that it's OK to access the memory or we might crash
        // iTunes.
        wchar_t current;
        wchar_t next;
        if ( TryMemory( pStart + count ) && TryMemory( pStart + count + 1 ) )
        {
            current = pStart[count];
            next = pStart[count + 1];
        }
        else
        {
            // We've triggered an invalid memory access. Stop.
            LOGL( 1, "Bad memory access stopped path search" );
            return NULL;
        }

        if( current == L':' && next == '\\' ||
            current == L'\\' && next == '\\' )
        {
            std::ostringstream os;
            os << "Path candidate found at offset " << count;
            LOGL( 3, os.str() );

            // Do an extra check here by searching for the filename
            // within MAX_PATH chars of this.
            std::wstring sPath(&pStart[count], MAX_PATH);
            if (sPath.find(fileName, 0) != std::wstring::npos)
            {
                LOGL( 3, "Candidate matched filename");
                if ( pStart[count] == L':' )
                {
                    // Drive letter is one step before our current pos
                    --count;
                }
                return &pStart[count];
            }
        }

        count += increment;
    }

    LOGL( 3, "No path found");
    return NULL;
}


static wchar_t*
SearchForPath( wchar_t* pStart, std::wstring& fileName )
{
    // This is a startlingly bad way to find the filename.
    // It's in memory somewhere around the current
    // play message, so we just saunter up there and get it :-)

    // Reason this is done is that pTrack->fileName
    // doesn't include the full path.

    // In iTunes < 7, the path seems to be ahead of this location,
    // in later versions it seems to be 289 bytes before it, so
    // let's try the version 7 method first.

    wchar_t* path = SearchForPathDirectional( pStart, fileName, -1, 1000 );

    if ( path == NULL )
        path =  SearchForPathDirectional( pStart, fileName, 1, 10000 );

    return path;
}


void
LogTrack()
{
    LOGWL( 3, "Artist: " << gArtist << "\n  " <<
        "Track: " << gTrack << "\n  " <<
        "Album: " << gAlbum << "\n  " <<
        "Length: " << gLen << "\n  " <<
        "Path: " << gPath << "\n  " <<
        "Kind: " << gKind << "\n  " <<
        "Bitrate: " << gBitrate );
}


/*******************************************************************************
 * Scrobbling functions                                                        *
 *******************************************************************************/

/** submits the current song to audioscrobbler */
static void ASStart()
{
    int id = gSubmitter.Start( Moose::wStringToUtf8( gArtist ),
                               Moose::wStringToUtf8( gTrack ),
                               Moose::wStringToUtf8( gAlbum ),
                               "",
                               gLen,
                               Moose::wStringToUtf8( gPath ),
                               ScrobSubmitter::UTF_8 );

    std::wostringstream os;
    os << L"ReqID " << id << ": " << L"Sent Start for "
       << gArtist << " - " << gTrack;
    LOGWL( 3, os.str() );

    gASState = AS_PLAYING;
}


static void ASStop()
{
    int id = gSubmitter.Stop();

    std::ostringstream os;
    os << "ReqID " << id << ": " << "Sent Stop";
    LOGL( 3, os.str() );

    gArtist = L"";
    gTrack = L"";

    gASState = AS_STOPPED;
}


static void ASPause()
{
    int id = gSubmitter.Pause();

    std::ostringstream os;
    os << "ReqID " << id << ": " << "Sent Pause";
    LOGL( 3, os.str());

    gASState = AS_PAUSED;
}


static void ASResume()
{
    int id = gSubmitter.Resume();

    LOGL( 3, "ReqID " << id << ": " << "Sent Resume" );

    gASState = AS_PLAYING;
}


void ScrobSubCallback(int reqID, bool error, std::string message, void* userData)
{
#ifdef _DEBUG
    if (error)
    {
        MessageBox( NULL, message.c_str(), NULL, MB_OK );
    }
#endif

    LOGL( 3, "ReqID " << reqID << ": " << message );
}


/*******************************************************************************
 * Our own functions                                                           *
 *******************************************************************************/

static void
HandleTrack( ITTrackInfo* pTrack )
{
    if ( pTrack == NULL )
        return;

    std::wstring artist = (wchar_t*)pTrack->artist;
    std::wstring track = (wchar_t*)pTrack->name;
    artist = Moose::fixStr(artist);
    track = Moose::fixStr(track);

    // On a 1x looped track, we will get a ChangeTrack event which means we
    // won't have gone into paused and will just drop through to the new
    // track code.

    bool didWeActuallyResume = false;
    if ( gASState == AS_PAUSED && 
       ( artist == gArtist && track == gTrack ) )
    {
        // We'll get here if the same track started again. If the elapsed
        // time since last stop is less than a second, we interpret it as
        // a restart, if it is longer, we interpret it as a resume.
        DWORD elapsed = GetTickCount() - gTimeOfLastStop;
        didWeActuallyResume = elapsed > 1000;

        LOGL( 3, "Resume check, elapsed: " << elapsed );
    }
        
    if ( didWeActuallyResume )
    {
        ASResume();
    }
    else
    {
        // track change, set globals
        ASStop();

        gArtist  = artist;
        gTrack = track;
        gAlbum = (wchar_t*)pTrack->album;
        gAlbum = Moose::fixStr(gAlbum);

        // "Audio CD Track"
        // "MPEG audio stream"
        // "MPEG audio track"...
        gKind = (wchar_t*)pTrack->kind;
        gKind = Moose::fixStr(gKind);

        gLen = pTrack->totalTimeInMS / 1000;
        gStars = pTrack->trackRating;
        gBitrate = pTrack->bitRate;

        std::wstring fileName = (wchar_t*)pTrack->fileName;
        fileName = Moose::fixStr(fileName);

        // What type of track is this?
        char type = '\0';
        if (pTrack->bitRate == 1411)
        {
            // deffo a CD
            type = 'c';
            LOGL( 3, "It's a CD");
        }
        else if (fileName.size() == 0)
        {
            // Filename size is also 0 when playing a file over local network...
            std::transform(gKind.begin(), gKind.end(), gKind.begin(), std::towupper); 
            if (gKind.find(L"STREAM") != std::wstring::npos)
            {
                type = 's';
                LOGL( 3, "It's a stream");
            }
            else
            {
                type = 'f';
                LOGL( 3, "It's a shared playlist file");
            }
        }
        else
        {
            // a music file
            type = 'f';
            LOGL( 3, "It's a file");
        }

        if (type == 's')
        {
            // We don't do streams
            return;
        }

        // Only try and find the path if it's a file as opposed to a stream or
        // a track off CD
        if (type == 'f')
        {
            // Check for video, not the most reliable as it relies on kind string
            //std::string sKind = CW2A(gKind.c_str());
            std::string sKind = "";
            transform(sKind.begin(), sKind.end(), sKind.begin(), tolower);
            if ( (sKind.find("video", 0) != std::string::npos) || 
                (sKind.find("movie", 0) != std::string::npos) )
            {
                LOGL( 3, "Video file, won't submit");
                return;
            }

            if ( fileName.size() > 0 )
            {
                wchar_t* pPath = SearchForPath((wchar_t*)pTrack->name, fileName);
                gPath = pPath == NULL ? fileName : pPath;
            }
            else
            {
                gPath = L"";
            }
        }
        else if (type == 'c')
        {
            gPath = L"cd";
        }
        else
        {
            gPath = L"";
        }

        LogTrack();

        ASStart();

        // Give this track to the ITunesComThread for syncing playcount with local db
        VisualPluginTrack vpt = {
            gArtist,
            gTrack,
            gAlbum,
            gPath,
            pTrack->playCount };
        
        // If iPod scrobbling is switched off, this pointer will not have been
        // initialised so we need to check it first.
        if ( gCom != 0 )
            gCom->syncTrack( vpt );
    }
}


/*******************************************************************************
 * iTunes Plugin callbacks                                                     *
 *******************************************************************************/

static void
ProcessRenderData( VisualPluginData *visualPluginData, const RenderVisualData *renderData )
{
    //  we don't do any complex processing with render data 
    //  at the moment - but it is an indication that we are playing
}


/** RenderVisualPort - this is the main drawing code */
static void
RenderVisualPort (VisualPluginData *visualPluginData, GRAPHICS_DEVICE destPort, const Rect *destRect, Boolean onlyUpdate)
{
    if (destPort == nil)
        return;

    HDC hdc = GetDC(destPort);

    HBRUSH brush = CreateSolidBrush(RGB(220, 220, 220));

    RECT client;
    client.bottom = destRect->bottom;
    client.top = destRect->top;
    client.left = destRect->left;
    client.right = destRect->right;

    // Paint background
    FillRect(hdc, &client, brush);

    // Draw version string
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT)) ;
    RECT textRect = {10, 10, 400, 30};
    std::ostringstream os;
    os << kVisualPluginName << " " << GetVersionString();
    DrawText(hdc, os.str().c_str(), -1, &textRect, DT_LEFT | DT_TOP);

    // Draw options box
    RECT boxRect = { destRect->right - 144, destRect->top + 10,
                     destRect->right - 10, destRect->top + 50 };
    DeleteObject(brush);
    brush = CreateSolidBrush(RGB(250, 250, 250));
    SelectObject(hdc, brush);
    //HPEN pen = GetStockObject(BLACK_PEN);
    SelectObject(hdc, GetStockObject(BLACK_PEN));
    Rectangle(hdc, boxRect.left - 4, boxRect.top - 4, boxRect.right + 4, boxRect.bottom + 4);
    DrawText(hdc, "Open the Last.fm\napplication to change your\nsettings.",
             -1, &boxRect, DT_LEFT | DT_TOP);

    DeleteObject(brush);
    ReleaseDC(destPort, hdc);
}


/** AllocateVisualData is where you should allocate any information that depends
  * on the port or rect changing (like offscreen GWorlds).
  */
static OSStatus
AllocateVisualData (VisualPluginData *visualPluginData, const Rect *destRect)
{
    return noErr;
}


/** DeallocateVisualData is where you should deallocate the things you have
  * allocated.
  */
static void
DeallocateVisualData (VisualPluginData *visualPluginData)
{}


static Boolean
RectanglesEqual(const Rect *r1, const Rect *r2)
{
    if (
        (r1->left == r2->left) &&
        (r1->top == r2->top) &&
        (r1->right == r2->right) &&
        (r1->bottom == r2->bottom)
       )
        return true;

    return false;
}


static OSStatus
ChangeVisualPort( VisualPluginData *visualPluginData, GRAPHICS_DEVICE destPort, const Rect *destRect )
{
    OSStatus        status;
    Boolean         doAllocate;
    Boolean         doDeallocate;

    status = noErr;

    doAllocate      = false;
    doDeallocate    = false;

    if (destPort != nil)
    {
        if (visualPluginData->destPort != nil)
        {
            if (RectanglesEqual(destRect, &visualPluginData->destRect) == false)
            {
                doDeallocate    = true;
                doAllocate      = true;
            }
        }
        else
        {
            doAllocate = true;
        }
    }
    else
    {
        doDeallocate = true;
    }

    if (doDeallocate)
        DeallocateVisualData(visualPluginData);

    if (doAllocate)
        status = AllocateVisualData(visualPluginData, destRect);

    if (status != noErr)
        destPort = nil;

    visualPluginData->destPort = destPort;
    if (destRect != nil)
        visualPluginData->destRect = *destRect;

    return status;
}


static void
ResetRenderData (VisualPluginData *visualPluginData)
{
    ClearMemory(&visualPluginData->renderData, sizeof(visualPluginData->renderData));
    ClearMemory(&visualPluginData->waveformData[0][0], sizeof(visualPluginData->waveformData));
    visualPluginData->level[0] = 0;
    visualPluginData->level[1] = 0;
}


OSStatus
VisualPluginHandler( OSType message, VisualPluginMessageInfo* messageInfo, void* refCon )
{
    OSStatus status = noErr;

    VisualPluginData* visualPluginData = (VisualPluginData*)refCon;

    #define _( x ) x: LOGL( 3, "EVENT: " #x );

    switch (message)
    {
        /** Sent when the visual plugin is registered.  The plugin should do 
          * minimal memory allocations here.  The resource fork of the plugin is 
          * still available. */
        case _(kVisualPluginInitMessage)
        {
            visualPluginData = (VisualPluginData *)malloc(sizeof(VisualPluginData));
            if (visualPluginData == nil)
            {
                status = memFullErr;
                break;
            }

            visualPluginData->appCookie = messageInfo->u.initMessage.appCookie;
            visualPluginData->appProc   = messageInfo->u.initMessage.appProc;

            // Remember the file spec of our plugin file. We need this so we can
            // open our resource fork during the configuration message

            messageInfo->u.initMessage.refCon = (void*) visualPluginData;
        }
        break;

        /** Sent when the visual plugin is unloaded */
        case _(kVisualPluginCleanupMessage)
        {
            ASStop();

            if (visualPluginData != nil)
                free( visualPluginData );
        }
        break;

        /** Sent when the visual plugin is enabled.  iTunes currently enables
          * all loaded visual plugins.  The plugin should not do anything here.
          */
        case _(kVisualPluginEnableMessage)
            break;

        case _(kVisualPluginDisableMessage)
            ASStop();
            break;

        /** Sent if the plugin requests idle messages.  Do this by setting the
          * kVisualWantsIdleMessages option in the 
          * PlayerRegisterVisualPluginMessage.options field. */
        case kVisualPluginIdleMessage:
        {
            //TODO this is called a lot, lets not do anything here..
            
            // The render msg will take care of refreshing the screen if we're playing
            if ( !visualPluginData->playing )
            {
                RenderVisualPort( visualPluginData, 
                                  visualPluginData->destPort,
                                  &visualPluginData->destRect,
                                  false );
            }
        }
        break;

        /** Sent if the plugin requests the ability for the user to configure 
          * it.  Do this by setting the kVisualWantsConfigure option in the 
          * PlayerRegisterVisualPluginMessage.options field. */
        case _(kVisualPluginConfigureMessage)
            break;

        /** Sent when iTunes is going to show the visual plugin in a port.  At
          * this point, the plugin should allocate any large buffers it needs.
          * Gets called once when window is first displayed. */
        case kVisualPluginSetWindowMessage:
        /** Sent when iTunes needs to change the port or rectangle of the
          * currently displayed visual. Resize. */
        case kVisualPluginShowWindowMessage:
        {
            LOGL( 3, "EVENT: kVisualPlugin*WindowMessage" );

            visualPluginData->destOptions = messageInfo->u.showWindowMessage.options;

            #if TARGET_OS_WIN32
                #define PORT messageInfo->u.setWindowMessage.window
            #else
                #define PORT messageInfo->u.setWindowMessage.port
            #endif

            status = ChangeVisualPort( visualPluginData,
                                       PORT,
                                       &messageInfo->u.showWindowMessage.drawRect );
            if (status == noErr)
            {
                RenderVisualPort( visualPluginData, visualPluginData->destPort, &visualPluginData->destRect, true);
            }
        }
        break;

        /** Sent when iTunes is no longer displayed. */
        case _(kVisualPluginHideWindowMessage)
        {
            ChangeVisualPort( visualPluginData, nil, nil );

            ClearMemory( &visualPluginData->trackInfo, sizeof(visualPluginData->trackInfo) );
            ClearMemory( &visualPluginData->streamInfo, sizeof(visualPluginData->streamInfo) );
        }
        break;

        /** Sent for the visual plugin to render a frame. Only get these when 
          * playing. */
        case kVisualPluginRenderMessage:
            visualPluginData->renderTimeStampID = messageInfo->u.renderMessage.timeStampID;
            RenderVisualPort(visualPluginData, visualPluginData->destPort, &visualPluginData->destRect, false);
            break;

        /** Sent in response to an update event.  The visual plugin should update
          * into its remembered port.  This will only be sent if the plugin has been
          * previously given a ShowWindow message. */  
        case kVisualPluginUpdateMessage:
            RenderVisualPort( visualPluginData, visualPluginData->destPort, &visualPluginData->destRect, true );
            break;

        /** Sent when the playback starts */
        case _(kVisualPluginPlayMessage)
            HandleTrack( messageInfo->u.playMessage.trackInfoUnicode );
            visualPluginData->playing = true;
            break;

        /* Sent when the player changes the current track information. This is
         * used when the information about a track changes, or when the CD moves
         * onto the next track. The visual plugin should update any displayed
         * information about the currently playing song. */
        case _(kVisualPluginChangeTrackMessage)
            HandleTrack( messageInfo->u.changeTrackMessage.trackInfoUnicode );
            break;

        /** Sent when the player stops. */
        case _(kVisualPluginStopMessage)
        {
            if ( gASState == AS_PLAYING )
            {
                gTimeOfLastStop = GetTickCount();
                ASPause();
            }

            visualPluginData->playing = false;

            ResetRenderData(visualPluginData);
            RenderVisualPort(visualPluginData, visualPluginData->destPort, &visualPluginData->destRect, true);
        }
        break;

        /** Sent when the player changes the track position. */
        case _(kVisualPluginSetPositionMessage)
            break;

        /** Sent when the player pauses.  iTunes does not currently use pause or 
          * unpause. A pause in iTunes is handled by stopping and remembering 
          * the position. */
        case _(kVisualPluginPauseMessage)
        {
            LOGL( 3, "***************************** PAUSE MESSAGE *****************************************" );
            if ( gASState == AS_PLAYING )
            {
                ASPause();
            }

            visualPluginData->playing = false;

            ResetRenderData( visualPluginData );
            RenderVisualPort( visualPluginData, visualPluginData->destPort, &visualPluginData->destRect, true );
        }
        break;

        /** Sent when the player unpauses.  iTunes does not currently use pause 
          * or unpause. A pause in iTunes is handled by stopping and remembering 
          * the position. */
        case _(kVisualPluginUnpauseMessage)
        {
            if (gASState == AS_PAUSED)
            {
                ASResume();
            }

            visualPluginData->playing = true;

            ResetRenderData( visualPluginData );
            RenderVisualPort( visualPluginData, visualPluginData->destPort, &visualPluginData->destRect, true );
        }
        break;

        /** Sent to the plugin in response to a MacOS event.  The plugin should 
          * return noErr for any event it handles completely, or an error
          * (unimpErr) if iTunes should handle it. */
        case kVisualPluginEventMessage:
        default:
            status = unimpErr;
            break;
    }

    #undef _ //logging helper macro

    return status;
}


BOOL APIENTRY
DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        // intial startup - null everything just for safety
        gHModule = hModule;
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        // er?
    }

    return TRUE;
}
