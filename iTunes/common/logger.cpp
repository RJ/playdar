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

#include "Logger.h"
#include <ctime>
#include <iostream>
#include <iomanip>
#include <cstdlib>

#ifndef WIN32
    #include <sys/stat.h>
    #include <pthread.h>
#endif

Logger* Logger::instance = 0;

Logger::Logger( const COMMON_CHAR* path, Severity severity ) 
      : mLevel( severity )
{
    using namespace std;
    
    instance = this;

#ifdef WIN32
    InitializeCriticalSection( &mMutex );
#else
    pthread_mutexattr_t attr;
    pthread_mutexattr_init( &attr );
    pthread_mutex_init( &mMutex, &attr );
#endif

#ifdef WIN32
    HANDLE hFile = CreateFileW( path,
                               GENERIC_READ,
                               0, 
                               NULL,
                               OPEN_EXISTING, 
                               FILE_ATTRIBUTE_NORMAL, 
                               NULL);
    long const fileSize = GetFileSize( hFile, NULL );
    CloseHandle( hFile );
#else
    struct stat st;
    long const fileSize = stat( path, &st ) == 0 ? st.st_size : 0;
#endif

    if ( fileSize > 500000 )
        truncate( path );

    ios::openmode flags = ios::out | ios::app;
    mFileOut.open( path, flags );

    if (!mFileOut)
    {
	#ifdef WIN32
		OutputDebugStringA( "Could not open log file:" );
		OutputDebugStringW( path );
	#else
		cerr << "Could not open log file" << path;
	#endif
        return;
    }
    
    mFileOut << endl << endl;
    mFileOut << "==========================================================================lastfm" << endl;
}


Logger::~Logger()
{
    mFileOut.close();
#ifdef WIN32
    DeleteCriticalSection( &mMutex );
#else
    pthread_mutex_destroy( &mMutex );
#endif
}


static inline std::string time()
{
    time_t now;
    time( &now );
    char s[128];
    strftime( s, 127, "%y%m%d %H:%M:%S", gmtime( &now ) );
    return std::string( s);
}


void
Logger::log( const char* message )
{
    if (!mFileOut)
        return;

#ifdef WIN32
    EnterCriticalSection( &mMutex );
#else
    pthread_mutex_lock( &mMutex );
#endif#include "lib/lastfm/core/CoreDir.h"
    mFileOut << "[" << time() << "] " << message << std::endl;

#ifdef WIN32
    LeaveCriticalSection( &mMutex );
#else
    pthread_mutex_unlock( &mMutex );
#endif
}


void
Logger::log( Severity level, const std::string& message, const char* function, int line )
{
    if (level > mLevel)
        return;

    std::ostringstream s;
    s << function << "()";
    if (level < mLevel)
        s << line << ": L" << level;
    s << std::endl;
    s << message << std::endl;

    log( s.str().c_str() );
}


#ifdef WIN32
void
Logger::log( Severity level, const std::wstring& in, const char* function, int line )
{
    // first call works out required buffer length
    int recLen = WideCharToMultiByte( CP_ACP, 0, in.c_str(), (int)in.size(), NULL, NULL, NULL, NULL );

    char* buffer = new char[recLen + 1];
    memset(buffer,0,recLen+1);

    // second call actually converts
    WideCharToMultiByte( CP_ACP, 0, in.c_str(), (int)in.size(), buffer, recLen, NULL, NULL );
    std::string message = buffer;
    delete[] buffer;

    log( level, message, function, line );
}
#endif


void
Logger::truncate( const COMMON_CHAR* path ) //static
{
    using namespace std;
    
    ifstream inFile( path );
    inFile.seekg( -400000, std::ios_base::end );
    istreambuf_iterator<char> bufReader( inFile ), end;
    string sFile;
    sFile.reserve( 400005 );
    sFile.assign( bufReader, end );
    inFile.close();
    ofstream outFile( path );
    outFile << sFile << flush;
    outFile.close();
}
