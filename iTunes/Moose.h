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

#ifndef MOOSE_H
#define MOOSE_H

#include <cstdio>
#include "common/string.h"

#ifdef WIN32
    #define UNICORN_HKEY L"Software\\Last.fm\\" 
    #define MOOSE_HKEY_A "Software\\Last.fm\\Last.fm"
    #define MOOSE_HKEY  L"Software\\Last.fm\\Last.fm"
    #define MOOSE_PLUGIN_HKEY_A MOOSE_HKEY_A "\\Plugins\\itw"
#else
    #define MOOSE_PREFS_PLIST "fm.last.Last.fm"
#endif


namespace Moose
{
    /** @returns  /-terminated utf8 encoded path */
    COMMON_STD_STRING
    applicationFolder();

    /** @returns /-terminated utf8 encoded path */
    COMMON_STD_STRING
    applicationSupport();

    /** @returns utf8 encoded Last.fm client application binary path */
    COMMON_STD_STRING
    applicationPath();    
    
    /** @returns utf8 encoded path */
    COMMON_STD_STRING
    twiddlyPath();

    /** @returns the complete path to the plugin library binary
      * 16-bit unicode on Windows and local 8-bit on Mac */
    COMMON_STD_STRING
    pluginPath();

    bool
    isTwiddlyRunning();

    /** on mac, calls setFileDescriptorsCloseOnExec() for you
      * on mac all parameters should be utf8 */
    bool
    exec( const COMMON_STD_STRING& command, const COMMON_STD_STRING& space_separated_args );
    
    bool
    iPodScrobblingEnabled();

    #if !defined WIN32
        /** returns true if the Moose setting for launch with media player is set */
        bool
        launchWithMediaPlayer();
        
        /** call when you exec something, otherwise you'll get error -54 */
        void
        setFileDescriptorsCloseOnExec();

        /** /-terminated */
        std::string
        bundleFolder();
    #endif

    #ifdef WIN32
        /** I'm not sure what this does really --mxcl */
        std::wstring
        fixStr( const std::wstring& );

        /** Converts our nice unicode to utf-8 using MS' horrific api call :) */
        std::string
        wStringToUtf8( const std::wstring& );
    #endif
}


#ifndef WIN32
    inline std::string 
    Moose::twiddlyPath()
    {
        return applicationFolder() + "../Resources/twiddly";
    }
#else
    inline std::wstring
    Moose::twiddlyPath()
    {
        return applicationFolder() + L"twiddly.exe";
    }
#endif

#endif //MOOSE_H
