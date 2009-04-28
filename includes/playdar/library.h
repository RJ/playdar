#ifndef __PLAYDAR_LIBRARY_H__
#define __PLAYDAR_LIBRARY_H__

#include <cstdio>
#include <map>
#include <vector>
#include <boost/thread/mutex.hpp>


#include "playdar/types.h"
#include "playdar/artist.h"
#include "playdar/album.h"
#include "playdar/track.h"
#include "playdar/library_file.h"

#include "sqlite3pp.h"

#include "playdar/streaming_strategy.h"
#include "playdar/ss_localfile.hpp"

namespace playdar {

class MyApplication;

class Library
{
public:
    Library(const std::string& dbfilepath);
    ~Library();

    int add_dir( const std::string& url, int mtime);
    int add_file( const std::string& url, int mtime, int size, const std::string& md5, const std::string& mimetype,
                  int duration, int bitrate,
                  const std::string& artist, const std::string& album, const std::string& track, int tracknum);
    
    bool remove_file( const std::string& url );

    int get_artist_id(const std::string&);
    int get_track_id(int, const std::string&);
    int get_album_id(int, const std::string&);

    std::map<std::string, int> file_mtimes();
    int num_files();
    int num_artists();
    int num_albums();
    int num_tracks();

    bool build_index(std::string);
    static std::string sortname(const std::string& name);
    std::map<std::string, int> ngrams(const std::string&);

    std::vector<scorepair> search_catalogue(std::string, std::string);
    std::vector<scorepair> search_catalogue_for_artist(int, std::string, std::string);

    // catalogue items
    artist_ptr  load_artist(std::string n);
    artist_ptr  load_artist(int n);
    inline static artist_ptr load_artist( sqlite3pp::database* db, int n )
    {
        sqlite3pp::query qry(*db, "SELECT id,name FROM artist WHERE id = ?");
        qry.bind(1, n);
        artist_ptr ptr;
        for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
            ptr = artist_ptr(new Artist((*i).get<int>(0), (*i).get<std::string>(1)));
            break;
        }
        return ptr;   
    }
    
    album_ptr   load_album(artist_ptr artp, std::string n);
    album_ptr   load_album(int n);
    inline static album_ptr load_album( sqlite3pp::database* db, int n )
    {
        sqlite3pp::query qry(*db, "SELECT id,name,artist FROM album WHERE id = ?");
        qry.bind(1, n);
        album_ptr ptr;
        for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
            ptr = album_ptr(new Album((*i).get<int>(0), (*i).get<std::string>(1), load_artist( db, (*i).get<int>(2))));
            break;
        }
        return ptr;   
    }
    
    track_ptr   load_track(artist_ptr artp, std::string n);
    track_ptr   load_track(int n);
    inline static track_ptr load_track( sqlite3pp::database* db, int n )
    {
        sqlite3pp::query qry(*db, "SELECT id,name,artist FROM track WHERE id = ?");
        qry.bind(1, n);
        track_ptr ptr;
        for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
            ptr = track_ptr(new Track((*i).get<int>(0), (*i).get<std::string>(1), load_artist(db, (*i).get<int>(2))));
            break;
        }
        return ptr;
    }

    // browsing:
    std::vector< boost::shared_ptr<Artist> > list_artists();
    std::vector< boost::shared_ptr<Track> > list_artist_tracks(boost::shared_ptr<Artist>);
    
    std::string get_name(std::string, int);
    std::string get_field(std::string, int, std::string);

    std::vector<int> get_fids_for_tid(int tid);
    LibraryFile_ptr file_from_fid(int fid);
    inline static LibraryFile_ptr file_from_fid( sqlite3pp::database* db, int fid )
    {
        sqlite3pp::query qry(*db,
                             "SELECT file.url, file.size, file.mimetype, file.duration, file.bitrate, "
                             "file_join.artist, file_join.album, file_join.track "
                             "FROM file, file_join "
                             "WHERE file.id = file_join.file "
                             "AND file.id = ?");
        qry.bind(1, fid);
        sqlite3pp::query::iterator i( qry.begin() );
        if (i == qry.end())
            return LibraryFile_ptr((LibraryFile*)0);
        
        LibraryFile_ptr p(new LibraryFile);
        p->url = std::string((*i).get<const char *>(0));
        p->size = (*i).get<int>(1);
        p->mimetype = std::string((*i).get<const char *>(2));
        p->duration = (*i).get<int>(3);
        p->bitrate = (*i).get<int>(4);
        p->piartid = (*i).get<int>(5);
        p->pialbid = (*i).get<int>(6);
        p->pitrkid = (*i).get<int>(7);
        return p;   
    }
    
    sqlite3pp::database * db() { return &m_db; }
    std::string dbfilepath() const { return m_dbfilepath; }
    
    // DB helper:
    template <typename T> T db_get_one(std::string sql, T def);
    
private:
    void check_db();
    void create_db_schema();
    sqlite3pp::database m_db;
    boost::mutex m_mut;
    std::string m_dbfilepath;
    // name -> id caches
    std::map< std::string, int > m_artistcache;
    std::map< int, std::map<std::string, int> > m_trackcache;
    std::map< int, std::map<std::string, int> > m_albumcache;
};

}

#endif

