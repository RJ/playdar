#include "playdar/library.h"

//#include <boost/asio.hpp>

#include <iostream>
#include <cstdio>
#include <sstream>

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "playdar/application.h"
#include "playdar/playable_item.hpp"

using namespace std;


Library::Library(const string& dbfilepath)
: m_db(dbfilepath.c_str())
{
    m_dbfilepath = dbfilepath;
}

Library::~Library()
{
    cout << "DTOR library" << endl;
}


bool
Library::remove_file( const string& url )
{
    boost::mutex::scoped_lock lock(m_mut);
    sqlite3pp::query qry(m_db, "SELECT id FROM file WHERE url = ?");
    qry.bind(1, url.c_str(), true);
    int fileid = 0;
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        fileid = (*i).get<int>(0);
        break; // should only be one row
    }
    if(fileid==0) return false;
    sqlite3pp::command cmd1(m_db, "DELETE FROM file_join WHERE file = ?");
    sqlite3pp::command cmd2(m_db, "DELETE FROM file WHERE id = ?");
    cmd1.bind(1, fileid);
    cmd2.bind(1, fileid);
    cmd1.execute();
    cmd2.execute();
    return true;
}

int 
Library::add_dir( const string& url, int mtime)
{
    boost::mutex::scoped_lock lock(m_mut);
    remove_file( url );
    sqlite3pp::command cmd(m_db, "INSERT INTO file(url, size, mtime) VALUES (?, 0, ?)");
    cmd.bind(1, url.c_str(), true);
    cmd.bind(2, mtime);
    return cmd.execute();        
}

int 
Library::add_file(  const string& url, int mtime, int size, const string& md5, const string& mimetype,
                    int duration, int bitrate,
                    const string& artist, const string& album, const string& track, int tracknum)
{
    int fileid = 0;
    remove_file(url);

    sqlite3pp::command cmd(m_db, "INSERT INTO file(url, size, mtime, md5, mimetype, duration, bitrate) VALUES (?, ?, ?, ?, ?, ?, ?)");
    cmd.bind(1, url.c_str(), true);
    cmd.bind(2, size);
    cmd.bind(3, mtime);
    cmd.bind(4, md5.c_str(), true);
    cmd.bind(5, mimetype.c_str(), true);
    cmd.bind(6, duration);
    cmd.bind(7, bitrate);
    if(cmd.execute() != SQLITE_OK){
        cerr<<"Error inserting into file table"<<endl;
        return 0;
    }
    fileid = static_cast<int>( m_db.last_insert_rowid() );
    int artid = get_artist_id(artist);
    if(artid<1){
        return 0;
    }
    int trkid = get_track_id(artid, track);
    if(trkid<1){
        return 0;
    }
    int albid = get_album_id(artid, album);
    // Now add the association
    sqlite3pp::command cmd2(m_db, "INSERT INTO file_join(file, artist ,album, track) VALUES (?,?,?,?)");
    cmd2.bind(1, fileid);
    cmd2.bind(2, artid);
    cmd2.bind(3, albid);
    cmd2.bind(4, trkid);
    boost::mutex::scoped_lock lock(m_mut);
    if(cmd2.execute() != SQLITE_OK){
        cerr<<"Error inserting into file_join table"<<endl;
        return 0;
    }
    return fileid; 
}

int
Library::get_artist_id(const string& name_orig)
{
    boost::mutex::scoped_lock lock(m_mut);
    int id = 0;
    string sortname = Library::sortname(name_orig);
    if((id = m_artistcache[sortname])) return id;
    sqlite3pp::query qry(m_db, "SELECT id FROM artist WHERE sortname = ?");
    qry.bind(1, sortname.c_str(), true);
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        id = (*i).get<int>(0);
        break; // should only be one row
    }
    if(id){
        //cout << "Hit: " << sortname << " == " << id << endl;
        m_artistcache[sortname]=id;
        return id;
    }
    // not found, insert it.
    sqlite3pp::command cmd(m_db, "INSERT INTO artist(id,name,sortname) VALUES(NULL,?,?)");
    cmd.bind(1,name_orig.c_str(),true);
    cmd.bind(2,sortname.c_str(),true);
    if(SQLITE_OK != cmd.execute()){
        cerr << "Failed to insert artist: " << name_orig << endl;
        return 0;
    }
    id = static_cast<int>( m_db.last_insert_rowid() );
    //cout << "New insert: " << sortname << " == " << id << endl;
    m_artistcache[sortname]=id;
    return id;
}

int
Library::get_track_id(int artistid, const string& name_orig)
{
    boost::mutex::scoped_lock lock(m_mut);
    int id = 0;
    string sortname = Library::sortname(name_orig);
    if((id = m_trackcache[artistid][sortname])) return id;
    sqlite3pp::query qry(m_db, "SELECT id FROM track WHERE artist = ? AND sortname = ?");
    qry.bind(1, artistid);
    qry.bind(2, sortname.c_str(), true);
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        id = (*i).get<int>(0);
        break; // should only be one row
    }
    if(id){
        //cout << "Hit: " << sortname << " == " << id << endl;
        m_trackcache[artistid][sortname]=id;
        return id;
    }
    // not found, insert it.
    sqlite3pp::command cmd(m_db, "INSERT INTO track(id,artist,name,sortname) VALUES(NULL,?,?,?)");
    cmd.bind(1, artistid);
    cmd.bind(2, name_orig.c_str(), true);
    cmd.bind(3, sortname.c_str(), true);
    if(SQLITE_OK != cmd.execute()){
        cerr << "Failed to insert track: " << name_orig << endl;
        return 0;
    }
    id = static_cast<int>( m_db.last_insert_rowid() );
    //cout << "New insert: " << sortname << " == " << id << endl;
    m_trackcache[artistid][sortname]=id;
    return id;
}

int
Library::get_album_id(int artistid, const string& name_orig)
{
    boost::mutex::scoped_lock lock(m_mut);
    int id = 0;
    string sortname = Library::sortname(name_orig);
    if((id = m_albumcache[artistid][sortname])) return id;
    sqlite3pp::query qry(m_db, "SELECT id FROM album WHERE artist = ? AND sortname = ?");
    qry.bind(1, artistid);
    qry.bind(2, sortname.c_str(), true);
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        id = (*i).get<int>(0);
        break; // should only be one row
    }
    if(id){
        //cout << "Hit: " << sortname << " == " << id << endl;
        m_albumcache[artistid][sortname]=id;
        return id;
    }
    // not found, insert it.
    sqlite3pp::command cmd(m_db, "INSERT INTO album(id,artist,name,sortname) VALUES(NULL,?,?,?)");
    cmd.bind(1, artistid);
    cmd.bind(2, name_orig.c_str(), true);
    cmd.bind(3, sortname.c_str(), true);
    if(SQLITE_OK != cmd.execute()){
        cerr << "Failed to insert album: " << name_orig << endl;
        return 0;
    }
    id = static_cast<int>( m_db.last_insert_rowid() );
    //cout << "New insert: " << sortname << " == " << id << endl;
    m_albumcache[artistid][sortname]=id;
    return id;
}

vector<scorepair>
Library::search_catalogue(string table, string name_orig)
{
    boost::mutex::scoped_lock lock(m_mut);
    vector<scorepair> results;
    if(table != "artist" && table != "track" && table != "album") return results;
    if(name_orig.length()<3) return results;
    string name = sortname(name_orig);
    map<string,int> ngrammap = ngrams(name);
    map<string,int>::const_iterator iter = ngrammap.begin();
    string q("?");
    iter++;
    for( ; iter!=ngrammap.end(); ++iter)
    {
        q += ", ?";
    }
    string sql = "SELECT s.id, sum(s.num) as score ";
    sql +=       "FROM " + table + "_search_index as s ";
    sql +=       "WHERE ngram IN (" + q + ") ";
    sql +=       "GROUP BY s.id ORDER BY sum(s.num) DESC LIMIT 10";
    sqlite3pp::query qry(m_db, sql.c_str());
    int numn = 0;
    for(iter = ngrammap.begin(); iter!=ngrammap.end(); ++iter){
        qry.bind(++numn, iter->first.c_str(), true);
    }
    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
        scorepair sp;
        sp.id = (*i).get<int>(0);
        sp.score = (float) (*i).get<int>(1);
        results.push_back(sp);
    }
    return results;
}

vector<scorepair>
Library::search_catalogue_for_artist(int artistid, string table, string name_orig)
{
    boost::mutex::scoped_lock lock(m_mut);
    vector<scorepair> results;
    if(table != "track" && table != "album") return results;
    if(name_orig.length()<3) return results;
    string name = sortname(name_orig);
    map<string,int> ngrammap = ngrams(name);
    map<string,int>::const_iterator iter = ngrammap.begin();
    string q("?");
    iter++;
    for( ; iter!=ngrammap.end(); ++iter)
    {
        q += ", ?";
    }
    string sql = "SELECT s.id, sum(s.num) as score ";
    sql +=       "FROM " + table + "_search_index as s ";
    sql +=       "JOIN " + table + " ON " +table+ ".id = s.id ";
    sql +=       "WHERE " + table +".artist = ? AND ";
    sql +=       "ngram IN (" + q + ") ";
    sql +=       "GROUP BY s.id ORDER BY sum(s.num) DESC LIMIT 10";
    sqlite3pp::query qry(m_db, sql.c_str());
    qry.bind(1, artistid);
    int numn = 1;
    for(iter = ngrammap.begin(); iter!=ngrammap.end(); ++iter){
        qry.bind(++numn, iter->first.c_str(), true);
    }
    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
        scorepair sp;
        sp.id = (*i).get<int>(0);
        sp.score = (float) (*i).get<int>(1);
        results.push_back(sp);
    }
    return results;
}

//BROWSING
vector<artist_ptr>
Library::list_artists()
{
    boost::mutex::scoped_lock lock(m_mut);
    vector<artist_ptr> results;
    string sql = "SELECT id ";
    sql +=       "FROM artist ";
    sql +=       "ORDER BY sortname ASC";
    sqlite3pp::query qry(m_db, sql.c_str());
    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
        results.push_back( load_artist((*i).get<int>(0)) );
    }
    return results;
}

vector<track_ptr> 
Library::list_artist_tracks(artist_ptr artist)
{
    boost::mutex::scoped_lock lock(m_mut);
    vector< boost::shared_ptr<Track> > results;
    string sql = "SELECT id ";
    sql +=       "FROM track ";
    sql +=       "WHERE artist = ? ";
    sql +=       "ORDER BY sortname ASC";
    sqlite3pp::query qry(m_db, sql.c_str());
    qry.bind(1, artist->id());
    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
        results.push_back( load_track( (*i).get<int>(0) ) );
    }
    return results;
}

vector<int>
Library::get_fids_for_tid(int tid)
{
    boost::mutex::scoped_lock lock(m_mut);
    vector<int> results;
    sqlite3pp::query qry(m_db, "SELECT file.id FROM file, file_join WHERE file_join.file=file.id AND file_join.track = ? ORDER BY bitrate DESC");
    qry.bind(1, tid);
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        results.push_back( (*i).get<int>(0) );
    }
    return results;
}

bool 
Library::build_index(string table)
{
    if(table != "artist" && table != "track" && table != "album") return false;
    
    boost::mutex::scoped_lock lock(m_mut);
    
    cout << "Building index for " << table << endl;
    string searchtable = table + "_search_index";
        m_db.execute(string("DELETE FROM "+searchtable).c_str());
    sqlite3pp::query qry(m_db, string("SELECT id, sortname FROM "+table).c_str());
    int num_names = 0;
    int num_ngrams = 0;
    int id;
    char const * name;
    
    sqlite3pp::command cmd_i(m_db, string(  "INSERT INTO "+searchtable+
                                            "(ngram, id, num) VALUES (?,?,?)").c_str() );

    sqlite3pp::command cmd_u(m_db, string(  "UPDATE "+searchtable+" SET num=num+? "+
                                            "WHERE ngram=? AND id=?").c_str());

    for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
        id   = (*i).get<int>(0);
        name = (*i).get<char const*>(1);
        num_names++;
        cmd_i.bind(2, id); // set id
        cmd_u.bind(3, id); // set id
        map<string,int> ngrammap = ngrams(name);
        for(map<string,int>::const_iterator it = ngrammap.begin(); it != ngrammap.end(); ++it)
        {
            num_ngrams++;
            cmd_u.bind(1, it->second);
            cmd_u.bind(2, it->first.c_str());
            cmd_u.execute();
            cmd_u.reset();
            if(m_db.changes()==0) { // update failed, do insert
                cmd_i.bind(1, it->first.c_str());
                cmd_i.bind(3, it->second);
                cmd_i.execute();
                cmd_i.reset();
            }            
        }
    }
    cout << "Finished indexing " << table << " - " << num_names <<" names, " << num_ngrams << " ngrams." << endl;
    return true;
}

// horribly inefficient:
map<string,int> 
Library::ngrams(const string& str_orig)
{
    int n=3;
    map<string,int> m;
    //TODO trim etc
    string str = " " +sortname(str_orig) +" ";
    size_t num = str.length() - (n-1);
    string ngram;
    for(size_t j = 0; j < num; j++){
         ngram = str.substr(j, n);
         if(m[ngram]) m[ngram]++;
         else m[ngram]=1;
    }
    return m;
}

int 
Library::num_files()
{
    return db_get_one(string("SELECT count(*) FROM file"), 0);
}
int 
Library::num_artists()
{
    return db_get_one(string("SELECT count(*) FROM artist"), 0);
}
int 
Library::num_albums()
{
    return db_get_one(string("SELECT count(*) FROM album"), 0);
}
int 
Library::num_tracks()
{
    return db_get_one(string("SELECT count(*) FROM track"), 0);
}

LibraryFile_ptr
Library::file_from_fid(int fid)
{
    boost::mutex::scoped_lock lock(m_mut);
    sqlite3pp::query qry(m_db,
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
    p->url = string((*i).get<const char *>(0));
    p->size = (*i).get<int>(1);
    p->mimetype = string((*i).get<const char *>(2));
    p->duration = (*i).get<int>(3);
    p->bitrate = (*i).get<int>(4);
    p->piartid = (*i).get<int>(5);
    p->pialbid = (*i).get<int>(6);
    p->pitrkid = (*i).get<int>(7);
    return p;
}

// get mtimes of all filesnames scanned
map<string, int>
Library::file_mtimes()
{
    boost::mutex::scoped_lock lock(m_mut);
    map<string, int> ret;
    sqlite3pp::query qry(m_db, "SELECT url, mtime FROM file");
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        ret[ string((*i).get<const char *>(0)) ] = (*i).get<int>(1);
    }
    return ret;
}

// get just one field from first/only row of a db query
template <typename T> T
Library::db_get_one(string sql, T def)
{
    boost::mutex::scoped_lock lock(m_mut);
    T val;
    sqlite3pp::query qry(m_db, sql.c_str());
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        val = (*i).get<T>(def);
        return val;
    }
    return def;
}

string
Library::get_name(string table, int id)
{
    return get_field(table, id, "name");
}

string
Library::get_field(string table, int id, string field)
{
    boost::mutex::scoped_lock lock(m_mut);
    sqlite3pp::query qry(m_db, string("SELECT "+field+" FROM "+table+" WHERE id = ?").c_str() );
    qry.bind(1, id);
    string result("");
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        result = string((*i).get<const char *>(0));
        break; // should only be one row
    }
    return result;
}

string
Library::sortname(const string& name)
{
    string data(name);
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    boost::trim(data);
    return data;
}

// CATALOGUE LOADING (TODO) some factory of singletons->shared pointers, so only one lookup
// and one instance of each artist, album, track exists at any one time. boost pool maybe.

artist_ptr
Library::load_artist(string n)
{
    string sortname = Library::sortname(n);
    sqlite3pp::query qry(m_db, "SELECT id,name FROM artist WHERE sortname = ?");
    qry.bind(1, sortname.c_str(), true);
    artist_ptr ptr;
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        ptr = artist_ptr(new Artist((*i).get<int>(0), (*i).get<string>(1)));
        break;
    }
    return ptr;
}

artist_ptr
Library::load_artist(int n)
{
    sqlite3pp::query qry(m_db, "SELECT id,name FROM artist WHERE id = ?");
    qry.bind(1, n);
    artist_ptr ptr;
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        ptr = artist_ptr(new Artist((*i).get<int>(0), (*i).get<string>(1)));
        break;
    }
    return ptr;
}

track_ptr
Library::load_track(artist_ptr artp, string n)
{
    string sortname = Library::sortname(n);
    sqlite3pp::query qry(m_db, "SELECT id,name FROM track WHERE artist = ? AND sortname = ?");
    qry.bind(1, artp->id());
    qry.bind(2, sortname.c_str(), true);
    track_ptr ptr;
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        ptr = track_ptr(new Track((*i).get<int>(0), (*i).get<string>(1), artp));
        break;
    }
    return ptr;
}

track_ptr
Library::load_track(int n)
{
    sqlite3pp::query qry(m_db, "SELECT id,name,artist FROM track WHERE id = ?");
    qry.bind(1, n);
    track_ptr ptr;
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        ptr = track_ptr(new Track((*i).get<int>(0), (*i).get<string>(1), load_artist((*i).get<int>(2))));
        break;
    }
    return ptr;
}

album_ptr
Library::load_album(artist_ptr artp, string n)
{
    string sortname = Library::sortname(n);
    sqlite3pp::query qry(m_db, "SELECT id,name FROM album WHERE artist = ? AND sortname = ?");
    qry.bind(1, artp->id());
    qry.bind(2, sortname.c_str(), true);
    album_ptr ptr;
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        ptr = album_ptr(new Album((*i).get<int>(0), (*i).get<string>(1), artp));
        break;
    }
    return ptr;
}

album_ptr
Library::load_album(int n)
{
    sqlite3pp::query qry(m_db, "SELECT id,name,artist FROM album WHERE id = ?");
    qry.bind(1, n);
    album_ptr ptr;
    for(sqlite3pp::query::iterator i = qry.begin(); i!=qry.end(); ++i){
        ptr = album_ptr(new Album((*i).get<int>(0), (*i).get<string>(1), load_artist((*i).get<int>(2))));
        break;
    }
    return ptr;
}

