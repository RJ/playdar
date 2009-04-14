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
#include "ITunesTrack.h"
#include "ITunesExceptions.h"
#include "common/logger.h"

#include <pthread.h>
#include <sqlite3.h>

pthread_mutex_t ITunesPlaysDatabase::s_mutex;


void //static
ITunesPlaysDatabase::init()
{
    ITunesPlaysDatabase().prepare();
    pthread_mutex_init( &s_mutex, NULL );
}


void //static
ITunesPlaysDatabase::finit()
{
    pthread_mutex_destroy( &s_mutex );
}


struct PThreadMutexLocker
{
    PThreadMutexLocker( pthread_mutex_t& mutex ) : m_mutex( &mutex )
    {
        pthread_mutex_lock( m_mutex );
    }
    
    ~PThreadMutexLocker()
    {
        pthread_mutex_unlock( m_mutex );
    }
    
private:
    pthread_mutex_t* const m_mutex;
};


void* //static
ITunesPlaysDatabase::sync( void* that )
{
    ExtendedITunesTrack& track = * (ExtendedITunesTrack*) that;

    LOG( 3, "Syncing " << track.path() );

    // On trackChange iTunes takes about 4 seconds to update the playCount for 
    // the previous track :(
    // Thus we try to obtain a new playCount for up to 60 seconds (4+8+16+32)
    // We stop then because if the user skips a track, playCount will never
    // change. However we try for up to 60 as we have no idea how long it may
    // take for iTunes to update its db.
    for( int s = 4; s < 128; s += s )
    {
        try 
        {
            // do as infrequently as possible because it spawns applescript
            int const diff = track.playCountDifference();
        
            if (diff)
            {
                ITunesPlaysDatabase db;
                
                // rationale for UPDATE: if the track is not in the db then it
                // is new since the last ipod sync. Thus it cannot be on the ipod
                // It will be automatically inserted with the current playcount
                // when the ipod is next synced and Twiddly is run.
                // NOTE db.playCount() will return -1 in this case so the logs
                // will be misleading, usually stating: play_count='0', but this
                // is fine since the UPDATE will fail in this case.
                char* format = "UPDATE itunes_db SET play_count='%d' WHERE persistent_id='%q'";
                char* token = sqlite3_mprintf( format,
                                               db.playCount( track ) + diff,
                                               track.persistentId().c_str() );
                db.query( token );
                sqlite3_free( token );
                break;
            }
        }
        catch( PlayCountException& )
        {
            LOG( 2, "Applescript failed to obtain playCount" );
            
            // NOTE our options here are an additional scrobble or potentially
            // lose scrobbles, the latter is perhaps better, as statistically
            // the user is unlikely to have played the track in question on his
            // iPod, however the code to achieve that would make this function
            // far less maintainable
        }
        
        LOG( 3, "Playcount unchanged - sleeping " << s << " seconds..." );
        sqlite3_sleep( s * 1000 );
    }

    delete &track;
    
    pthread_exit( 0 );
}


void* //static
ITunesPlaysDatabase::onPlayStateChanged( void* )
{
    /** Behaviour:
      * Playback started: get current track, exit
      * Track changed: sync previous track, get current track, exit
      * Playback stopped (playlist ended): sync last track, clear s_track
      */

    static ExtendedITunesTrack s_track;
    ExtendedITunesTrack currentTrack = ExtendedITunesTrack::currentTrack();

    {
        PThreadMutexLocker locker( s_mutex );

        if (!s_track.isNull())
        {
            // s_track is the track that has just finished playing
            pthread_t thread;
            pthread_create( &thread, NULL, sync, new ExtendedITunesTrack( s_track ) );
        }
        else
            LOG( 3, "Track is null, won't sync" );

        s_track = currentTrack;
    }
    
    pthread_exit( 0 );
}


void //static
ITunesPlaysDatabase::sync()
{
    pthread_t thread;
    pthread_create( &thread, NULL, onPlayStateChanged, 0 );
}
