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

#include "ITunesTrack.h"
#include "common/logger.h"

#include <cassert>
#include <errno.h>

#ifdef WIN32
    #include "ITunesComWrapper.h"
    #include <atlcomtime.h> // for COleDateTime
    #include <iostream>
    #include <iomanip>
    #include <sstream>
#endif

using namespace std;


long stdStringToLong( const std::string& s ) throw( PlayCountException )
{
    long c = std::strtol( s.c_str(), 0, 10 /*base*/ );
    if ( errno == EINVAL )
        throw PlayCountException();
    return c;
}


#ifdef WIN32
ITunesTrack::ITunesTrack() :
    m_comTrack( 0 )
{}


ITunesTrack::ITunesTrack( IITTrack* track ) :
    m_comTrack( track )
{
    assert( track != 0 );

    m_path = pathForTrack( track );
    m_id = L"-1";
}


ITunesTrack::ITunesTrack( const ITunesTrack& that )
{
    clone( that );
}


ITunesTrack&
ITunesTrack::operator=( const ITunesTrack& that )
{
    // Check for self-assignment
    if ( &that != this )
    {
        clone( that );
    }

    return *this;
}


void
ITunesTrack::clone( const ITunesTrack& that )
{
    m_comTrack = that.m_comTrack;
    m_id = that.m_id;
    m_path = that.m_path;

    if ( m_comTrack != 0 )
        m_comTrack->AddRef();
}


ITunesTrack::~ITunesTrack()
{
    if ( m_comTrack != 0 )
        m_comTrack->Release();
}
#endif // WIN32


#ifndef WIN32
static void splitTheseFourCommaSeparatedParts( const std::string& in, std::string out[] )
{
    uint first = 0;
    for (int x = 0; x < 3; ++x)
    {   
        uint const second = in.find( ',', first );

        if (second == std::string::npos)
            throw PlayCountException();

        out[x] = in.substr( first, second - first );

        first = second + 2;
    }
    
    out[3] = in.substr( first );
}


ExtendedITunesTrack
ExtendedITunesTrack::currentTrack()
{
    ExtendedITunesTrack t;
    
    try
    {
        //TODO may as well precompile 3 scripts and then run them with carbon
        // that should be most efficient computationally and easiest to read method
        
        // returns eg. "412EAE1061ABF7D2, 6152, 3, /music/file.mp3"
        std::string const s = scriptResult( "currentTrack.scpt" );

        std::string parts[4];
        splitTheseFourCommaSeparatedParts( s, parts );
        
        t.m_id = parts[0];
        t.m_dbid = parts[1];
        t.m_path = parts[3];
        // may throw PlayCountException, if so initialPlayCount stays at -1
        // this always happens when iTunes is stopped
        t.m_initialPlayCount = stdStringToLong( parts[2] );
    }
    catch (PlayCountException&)
    {
        LOG( 3, "Couldn't get current track data" );
    }
    
    return t;
}
#endif
    

long
ITunesTrack::playCount() const throw( PlayCountException )
{
#ifdef WIN32
    if ( m_comTrack == 0 )
        return -1; // empty track case

    long cnt;
    HRESULT res = m_comTrack->get_PlayedCount( &cnt );
    try { ITunesComWrapper::handleComResult( res, L"Failed to get play count for track" ); }
    catch( ITunesException& ) { throw PlayCountException(); }

    return cnt;

#else
    std::string s = scriptResult( "playCountForDatabaseId.scpt", m_dbid );

    //std::string s = scriptResult( "playCountForPersistentId.scpt", m_id );    
    return stdStringToLong( s ); //may throw
#endif
}


#ifdef WIN32
wstring
ITunesTrack::track() const
{
    if ( m_comTrack == 0 )
        return L"";  // empty track case

    BSTR bstrName = 0;
    HRESULT res = m_comTrack->get_Name( &bstrName );
    ITunesComWrapper::handleComResult( res, L"Failed to read title of track" );

    return ITunesComWrapper::bstrToWString( bstrName );
}
#endif


#ifdef WIN32
wstring
ITunesTrack::artist() const
{
    if ( m_comTrack == 0 )
        return L"";  // empty track case

    BSTR bstrArtist = 0;
    HRESULT res = m_comTrack->get_Artist( &bstrArtist );
    ITunesComWrapper::handleComResult( res, L"Failed to read artist of track" );

    return ITunesComWrapper::bstrToWString( bstrArtist );
}
#endif


#ifdef WIN32
wstring
ITunesTrack::album() const
{
    if ( m_comTrack == 0 )
        return L"";  // empty track case

    BSTR bstrAlbum = 0;
    HRESULT res = m_comTrack->get_Album( &bstrAlbum );
    ITunesComWrapper::handleComResult( res, L"Failed to read album of track" );

    return ITunesComWrapper::bstrToWString( bstrAlbum );
}
#endif


#ifdef WIN32
wstring
ITunesTrack::lastPlayed() const
{
    if ( m_comTrack == 0 )
        return L"";  // empty track case

    DATE date; // this is a double
    HRESULT res = m_comTrack->get_PlayedDate( &date );
    ITunesComWrapper::handleComResult( res, L"Failed to read last played date of track" );

    // We get a date of 0.00000 if the track has never been played
    if ( date < 0.00001 && date > -0.00001 )
        return L"";

    COleDateTime oleDate( date );
    if ( oleDate.GetStatus() == COleDateTime::valid )
    {
        wostringstream os;
        os << setfill(L'0')
           << setw(4) << oleDate.GetYear() << "-" << setw(2) << oleDate.GetMonth() << "-" << setw(2) << oleDate.GetDay() 
           << L" "
           << setw(2) << oleDate.GetHour() << ":" << setw(2) << oleDate.GetMinute() << ":" << setw(2) << oleDate.GetSecond();
        
        return os.str();
    }
    else
    {
        throw ITunesException( "Failed to read last played date of track" );
    }
}
#endif


#ifdef WIN32
long
ITunesTrack::duration() const
{
    if ( m_comTrack == 0 )
        return -1;  // empty track case

    long duration = 0;
    HRESULT res = m_comTrack->get_Duration( &duration );
    ITunesComWrapper::handleComResult( res, L"Failed to read duration of track" );

    return duration;
}
#endif

#ifdef WIN32
bool
ITunesTrack::isSameAs( const ITunesTrack& that )
{
    if ( m_comTrack == 0 || that.m_comTrack == 0 )
        return false;

    long myDbId = 0;
    long theirDbId = 0;
    
    HRESULT res = m_comTrack->get_TrackDatabaseID( &myDbId );
    ITunesComWrapper::handleComResult( res, L"Failed to read db ID of track" );
    
    res = that.m_comTrack->get_TrackDatabaseID( &theirDbId );
    ITunesComWrapper::handleComResult( res, L"Failed to read db ID of track" );

    return myDbId == theirDbId;
}  
#endif

bool
ITunesTrack::isNull() const
{ 
  #ifdef WIN32
    return m_comTrack == 0 || m_path.empty();
  #else
    return m_id == "";
  #endif
}


#ifndef WIN32
#include "Moose.h"

std::string //static
ITunesTrack::scriptResult( const char* filename, const std::string& argv1 ) throw()
{
    LOG( 4, "Executing script: `" << filename << "' `" << argv1 << '\'' );
    
    std::string command;
    command += "osascript '";
    command += Moose::bundleFolder() + "Contents/Resources/";
    command += filename;
    command += "'";
    
    if ( argv1.size() )
    {
        command += ' ' + argv1;
    }
    
    // avoid iTunes error -54
    Moose::setFileDescriptorsCloseOnExec();
    
    FILE* pipe = ::popen( command.c_str(), "r" );
    if ( !pipe )
        return "ERROR";
    
    char buf[ 128 ];
    std::string out;
    while( fgets( buf, 128, pipe ) )
    {
        out += buf;
    }
    ::pclose( pipe );
    
    if ( out.length() ) {
        out.resize( out.length() - 1 ); // last character is always a \n
        LOG( 4, "Script result: `" << out << '\'' );
    }
    
    return out;
}
#endif


#ifdef WIN32
wstring
ITunesTrack::pathForTrack( IITTrack* track )
{
    wstring path;
    IITFileOrCDTrack* fileTrack = 0;
    HRESULT res = track->QueryInterface( IID_IITFileOrCDTrack, (void**)&fileTrack );
    if ( res != S_OK || fileTrack == 0 )
    {
        ITunesComWrapper::logComError( res, L"Casting IITrack to IITFileOrCDTrack failed" );
    }
    else
    {
        BSTR bstrLocation = 0; // BSTR = WCHAR*
        res = fileTrack->get_Location( &bstrLocation );
        if ( res == S_OK )
        {
            path = ITunesComWrapper::bstrToWString( bstrLocation );
        }
        else
        {
            BSTR a;
            BSTR n;
            fileTrack->get_Artist( &a );
            fileTrack->get_Name( &n );
            wstring artist = ITunesComWrapper::bstrToWString( a );
            wstring track = ITunesComWrapper::bstrToWString( n );
            wostringstream os;
            os << L"COM couldn't get file path for " << artist << L" - " << track;
            ITunesComWrapper::logComError( res, os.str() );
        }

        fileTrack->Release();
    }

    return path;
}
#endif


int
ExtendedITunesTrack::playCountDifference() const throw( PlayCountException )
{
    assert( m_initialPlayCount != -2 ); // the default value for a null track
    
    int const playCount = this->playCount();

    // COM or Applescript failed :(
    if ( m_initialPlayCount == -1 || playCount == -1 )
        throw PlayCountException();

    return playCount - m_initialPlayCount;
}
