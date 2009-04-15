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

#include "ITunesPlaysDatabase.h"

#include "Moose.h"
#include "common/logger.h"
#include "ITunesTrack.h"

#include <errno.h>
#include <sys/stat.h>

#ifndef WIN32
    #include <unistd.h>
    #include <sqlite3.h>
    
    // this function is apparently not in the library deployed with OSX
    int sqlite3_sleep( int ms )
    {
        return ::usleep( ms * 1000 );
    }
#else
    #include "sqlite3.h"
#endif

// this is a macro to ensure useful location information in the log
#define logError( db ) \
        LOG( 2, "sqlite error: " \
            << sqlite3_errcode( db ) \
            << " (" << sqlite3_errmsg( db ) << ")" );


ITunesPlaysDatabase::ITunesPlaysDatabase()
{
#ifdef WIN32
    std::string path = Moose::wStringToUtf8( Moose::applicationSupport() + L"\\iTunesPlays.db" );
#else
    std::string path = Moose::applicationSupport() + "iTunesPlays.db";
#endif

    if ( sqlite3_open( path.c_str(), &m_db ) != SQLITE_OK )
    {
        LOG( 2, "ERROR: Could not open iTunesPlays Database: " << path );
        logError( m_db );
    }

#ifdef WIN32
    prepare();
#endif
}


void
ITunesPlaysDatabase::prepare()
{
	#ifdef WIN32
		#define BOOTSTRAP_FLAG L"--bootstrap"
	#else
		#define BOOTSTRAP_FLAG "--bootstrap"
	#endif

    // if twiddly is already running, quite probably we're already bootstrapping
    if ( (!isValid() || needsBootstrap()) && !Moose::isTwiddlyRunning() )
		Moose::exec( Moose::twiddlyPath(), BOOTSTRAP_FLAG );
}


ITunesPlaysDatabase::~ITunesPlaysDatabase()
{
    sqlite3_close( m_db ); //docs says passing NULL is harmless noop
}


bool
ITunesPlaysDatabase::query( /* utf-8 */ const char* statement, std::string* result )
{
    LOG( 3, statement );
    
    int error = 0;
    sqlite3_stmt* stmt = 0;

    try
    {
        const char* tail;
        error = sqlite3_prepare( m_db, statement, static_cast<int>( strlen( statement ) ), &stmt, &tail );
        
        if ( error != SQLITE_OK )
            throw "Could not compile SQL to byte-code";        
        
        int busyCount = 0;
        while( error != SQLITE_DONE && busyCount < 20 )
        {
            error = sqlite3_step( stmt );

            switch( error )
            {
                case SQLITE_BUSY:
                    busyCount++;
                    sqlite3_sleep( 25 );
                    break;

                case SQLITE_MISUSE:
                    throw "Could not execute SQL - SQLITE_MISUSE";

                case SQLITE_ERROR:
                    throw "Could not execute SQL - SQLITE_ERROR";

                case SQLITE_ROW:
                    if ( sqlite3_column_count( stmt ) > 0 && result )
                    {
                        *result = (const char*) sqlite3_column_text( stmt, 0 );
                        LOG( 3, "SQLite result: `" << *result << '\'' );
                    }
                    break;

                case SQLITE_DONE:
                    break;

                default:
                    throw "Unhandled sqlite3_step() return value";
            }
        }
        
        if ( busyCount )
        {
            if ( busyCount >= 20 )
                throw "Could not execute SQL - SQLite was too busy";
            
            LOG( 3, "SQLite was busy " << busyCount << " times" );
        }
    }
    catch( const char* message )
    {
        LOG( 2, "ERROR: " << message );
        logError( m_db );
    }

    sqlite3_finalize( stmt );

    return error == SQLITE_DONE;
}


#include "common/fileCreationTime.cpp"


bool
ITunesPlaysDatabase::isValid()
{
    if (!query( "SELECT COUNT( * ) FROM itunes_db LIMIT 0, 1;" ))
        return false;

    // uninstallation protection
    // we store the ctime in the database, so if the database is removed we
    // don't have this value still stored, which may cause additional issues
    // the principle is, if the plugin is uninstalled, then reinstalled sometime
    // the ctime will be different to what we stored at the time of bootstrap
    // the reason for all this is, if there was a period of uninstallation
    // in which the db wasn't removed (happens everytime on Windows), we don't
    // want to scrobble everything played since the uninstallation!
    std::string r;
    query( "SELECT value FROM metadata WHERE key='plugin_ctime';", &r );

    long const stored_time = std::strtol( r.c_str(), 0, 10 /*base*/ );
    time_t actual_time = common::fileCreationTime( Moose::pluginPath() );

    LOG( 3, "Plugin binary ctime: " << actual_time );

    if (errno == EINVAL || stored_time != actual_time)
    {
        LOG( 3, "Uninstallation protection triggered; forcing bootstrap" );
        return false;
    }

    return true;
}


bool
ITunesPlaysDatabase::needsBootstrap()
{
    std::string out;
    bool b = query( "SELECT value FROM metadata WHERE key='bootstrap_complete'", &out );
    return !(b && out == "true");
}


int
ITunesPlaysDatabase::playCount( const ITunesTrack& track )
{
#ifdef WIN32
    std::string id = Moose::wStringToUtf8( track.path() );
	char* format = "SELECT play_count FROM itunes_db WHERE path='%q'";
#else
	std::string id = track.persistentId();
	char* format = "SELECT play_count FROM itunes_db WHERE persistent_id='%q'";
#endif

    char* token = sqlite3_mprintf( format, id.c_str() );
    std::string result;
    
    if ( !query( token, &result ) )
        return -1;
    
    long c = std::strtol( result.c_str(), 0, 10 /*base*/ );
    if ( errno == EINVAL )
        return -1;
    return c;
}
