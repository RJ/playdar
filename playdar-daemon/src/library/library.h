#ifndef __PLAYDAR_LIBRARY_H__
#define __PLAYDAR_LIBRARY_H__

#include <iostream>
#include <stdio.h>
#include <map>
#include <vector>
#include "sqlite3pp.h"

using namespace std;

#include "application/types.h"
#include "library/artist.h"
#include "library/album.h"
#include "library/track.h"

#include "resolvers/streaming_strategy.h"
#include "resolvers/ss_localfile.h"

class MyApplication;

class Library
{
public:
    Library(string dbfilepath, MyApplication * a)
    : m_db(dbfilepath.c_str())
    {
        m_app = a;
    }

    ~Library()
    {
    }

    int add_dir(string, int);
    int add_file(string, int, int, string, string, int, int, string, string, string, int);
    bool remove_file(string);

    int get_artist_id(string);
    int get_track_id(int, string);
    int get_album_id(int, string);

    map<string,int> file_mtimes();
    int num_files();
    int num_artists();
    int num_albums();
    int num_tracks();

    bool build_index(string);
    static string sortname(string name);
    map<string,int> ngrams(string);

    vector<scorepair> search_catalogue(string, string);
    vector<scorepair> search_catalogue_for_artist(int, string, string);

    // catalogue items
    artist_ptr  load_artist(string n);
    artist_ptr  load_artist(int n);
    
    album_ptr   load_album(artist_ptr artp, string n);
    album_ptr   load_album(int n);
    
    track_ptr   load_track(artist_ptr artp, string n);
    track_ptr   load_track(int n);

    // browsing:
    vector< boost::shared_ptr<Artist> > list_artists();
    vector< boost::shared_ptr<Track> > list_artist_tracks(boost::shared_ptr<Artist>);
    
    boost::shared_ptr<PlayableItem> playable_item_from_fid(int fid);
    
    string get_name(string, int);
    string get_field(string, int, string);

    vector<int> get_fids_for_tid(int tid);

    MyApplication * app() { return m_app; }

    MyApplication * m_app;
    
    sqlite3pp::database * db(){ return &m_db; }
    
    // DB helper:
    template <typename T> T db_get_one(string sql, T def);
    
private:
    sqlite3pp::database m_db;
    boost::mutex m_mut;
    // name -> id caches
    map< string, int > m_artistcache;
    map< int, map<string, int> > m_trackcache;
    map< int, map<string, int> > m_albumcache;
};

#endif

