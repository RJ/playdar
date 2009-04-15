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

#include "Moose.h"
#include "app/moose.h"
#include <Windows.h>
#include <ShFolder.h>
#include <wchar.h>
#include "common/c++/Logger.h"
#include "RegistryUtils.h" // part of ScrobSub

using namespace::std;


std::wstring
Moose::applicationSupport()
{
    // This might not work on Win98 and earlier but we're not officially
    // supporting them anyway. Upgrading to IE5 will solve the problem.
    wchar_t acPath[MAX_PATH];
    HRESULT h = SHGetFolderPathW( NULL, 
                                 CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE,
                                 NULL, 
                                 0, 
                                 acPath );
    if ( h != S_OK )
        wcscpy( acPath, L"C:" );

    return std::wstring( acPath ) + L"\\Last.fm\\Client";
}


/** I'm not sure what this does really --mxcl */
std::wstring
Moose::fixStr(const std::wstring& str)
{
    std::wstring ret;
    
    if (str.length() == 0) return str;
    
    // Mac strings store the length in the first char of the string:
    size_t len = (size_t)str[0];
    ret = str.substr(1);
    if(len > 0 && len < ret.length())
    {
        ret = ret.substr(0,len);
    }
    return ret;
}


/** Converts our nice unicode to utf-8 using MS' horrific api call :) */
std::string
Moose::wStringToUtf8( const std::wstring& wideStr )
{
    // first call works out required buffer length
    int recLen = WideCharToMultiByte(CP_UTF8,0,wideStr.c_str(),(int)wideStr.length(),NULL,NULL,NULL,NULL);
    
    char* buffer = new char[recLen + 1];
    memset(buffer,0,recLen+1);
    
    //  second call actually converts
    WideCharToMultiByte(CP_UTF8,0,wideStr.c_str(),(int)wideStr.length(),buffer,recLen,NULL,NULL);
    
    std::string ret = buffer;
    
    // fuck <-- this comment for the sake of GoogleCode profanity searches
    delete[] buffer;
    
    return ret;
}


    std::wstring
    Moose::applicationPath()
    {
        HKEY h;
        LONG lResult = RegOpenKeyExA(
            HKEY_CURRENT_USER, 
            MOOSE_HKEY_A,
            0,              // reserved
            KEY_READ, // access mask
            &h );

        wchar_t buffer[MAX_PATH];
        buffer[0] = L'\0';

        if ( lResult == ERROR_SUCCESS )
        {
            try
            {
                RegistryUtils::QueryString( h, L"Path", buffer, MAX_PATH, false );
            }
            catch ( const RegistryUtils::CRegistryException& )
            {
                LOGL( 2, "Client path not found in HKCU" );
            }

            RegCloseKey( h );
        }

        if ( buffer[0] == L'\0' )
        {
            // Couldn't read path from HKCU, try HKLM
            lResult = RegOpenKeyExA(
                HKEY_LOCAL_MACHINE,
                MOOSE_HKEY_A,
                0,              // reserved
                KEY_READ,       // access mask
                &h);
        
            if ( lResult == ERROR_SUCCESS )
            {
                try
                {
                    RegistryUtils::QueryString( h, L"Path", buffer, MAX_PATH, false );
                }
                catch ( const RegistryUtils::CRegistryException& )
                {
                    LOGL( 2, "Client path not found in HKLM" );
                }

                RegCloseKey( h );
            }
        }

        if ( buffer[0] == L'\0' )
        {
            LOGL( 1, "Couldn't read the client path from the registry.");
            return std::wstring();
        }    

        return buffer;
    }


    std::wstring
    Moose::applicationFolder()
    {
        std::wstring result = Moose::applicationPath();

        size_t pos = result.rfind( '\\' );
        if ( pos == wstring::npos )
            pos = result.rfind( '/' );
            
        if ( pos == wstring::npos )
        {
            LOGL( 2, "Path to exe invalid" );
            return std::wstring();
        }
        
        return result.substr( 0, pos + 1 );
    }


bool
Moose::exec( const std::wstring& command, const std::wstring& args )
{
    LOGWL( 3, "Launching `" << command << ' ' << args << '\'' );
    HINSTANCE h = ShellExecuteW( NULL,
                                L"open",
                                command.c_str(),
                                args.c_str(),
                                NULL,
                                SW_SHOWNORMAL );

    if ( h <= reinterpret_cast<void*>( 32 ) )
    {
        LOGWL( 3, "Failed launching `" << command <<  "'\n. ShellExecute error: " << h );
        return false;
    }

    return true;
}


bool
Moose::isTwiddlyRunning()
{
    bool found = false;

    HANDLE mutex = ::CreateMutexA(  NULL,
                                    false,
                                    "Twiddly-05F67299-64CC-4775-A10B-0FBF41B6C4D0" );
    
    DWORD const e = ::GetLastError();
    if( e == ERROR_ALREADY_EXISTS || e == ERROR_ACCESS_DENIED )
    {
        //Can't create the mutex so twiddly must be running
        found = true;
    }

    //close the handle so that twiddly can create the mutex
    BOOL success = ::CloseHandle( mutex );
    LOGL( 3, "Twiddly mutex closed: " << (success ? "true" : "false") );

    if ( found ) LOGL( 3, "Twiddly already running!" );

    return found;
}


std::wstring
Moose::pluginPath()
{
    std::wstring path = L"C:\\Program Files\\iTunes\\Plug-Ins\\itw_scrobbler.dll";
    HKEY h = NULL;
    
    try
    {
        LONG l = RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                                MOOSE_PLUGIN_HKEY_A,
                                0,              // reserved
                                KEY_READ,       // access mask
                                &h );

        if (l == ERROR_SUCCESS)
        {
            wchar_t buffer[MAX_PATH];
            RegistryUtils::QueryString( h, 
                                        L"Path", 
                                        buffer, 
                                        MAX_PATH, 
                                        false, 
                                        path.c_str() /*default*/ );
            path = buffer;
        }
    }
    catch (RegistryUtils::CRegistryException&)
    {}
    
    RegCloseKey( h );
    
    return path;
}


bool
Moose::iPodScrobblingEnabled()
{
    bool b = true;
    HKEY h = NULL;

    try
    {
        LONG l = RegOpenKeyExA( HKEY_CURRENT_USER,
                                MOOSE_HKEY_A,
                                0,            // reserved
                                KEY_READ,     // access mask
                                &h );

        if (l == ERROR_SUCCESS)
        {
            // Qt stores booleans as strings! :(
            wchar_t buffer[12];
            RegistryUtils::QueryString( h, L"iPodScrobblingEnabled", buffer, 12, false, L"true" /*default*/ );
            b = wcscmp( buffer, L"true" ) == 0;
        }
    }
    catch (RegistryUtils::CRegistryException&)
    {}

    RegCloseKey( h );

    return b;
}