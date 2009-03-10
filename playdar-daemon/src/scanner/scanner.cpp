#include "playdar/application.h"
#include <iostream>
#include <cstdio>

#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <boost/filesystem.hpp>
#include <boost/exception.hpp>
#include <boost/algorithm/string.hpp>

#include <sqlite3.h>
#include "playdar/library.h"

using namespace std;
using namespace boost;
namespace bfs = boost::filesystem; 

bool add_file(const bfs::path &, int mtime);
bool add_dir(const bfs::path &);
string ext2mime(string ext);

Library *gLibrary;

int scanned, skipped, ignored = 0;


bool scan(const bfs::path &p, map<string,int> & mtimes)
{
    try
    {
        map<string, int>::iterator mtimeit;
        if(!exists(p)) return false;
        bfs::directory_iterator end_itr;
        for(bfs::directory_iterator itr( p ); itr != end_itr; ++itr){
            if ( bfs::is_directory(itr->status()) ){
                cout << "DIR: " << itr->path() << endl;
                scan(itr->path(), mtimes);
            } else {
                // is this file an audio file we understand?
                string extu(bfs::extension(itr->path()));
                string ext = to_lower_copy(extu);
                if( ext == ".mp3" ||
                    ext == ".m4a" || 
                    ext == ".mp4" ||  
                    ext == ".aac" )
                {
                    int mtime = bfs::last_write_time(itr->path());
                    mtimeit = mtimes.find(itr->string());
                    if(    mtimeit == mtimes.end() // not scanned previously
                        || mtimes[itr->string()] != mtime) // modified since last time
                    {
                        add_file(itr->path(), mtime);
                    }else{
                        ignored++;
                    }
                } else {
                    ignored++;
                    //cout << "Ignoring: " << itr->string() << endl;
                }
            }
        }
    }
    catch(std::exception const& e) { cerr << e.what() << endl;}
    //catch(...) { cout << "Fail at " << p.string() << endl; }

    return true;
}

bool add_dir(const bfs::path &p)
{
    try
    {
        int mtime = bfs::last_write_time(p);
        gLibrary->add_dir(p.string(), mtime);
        return true;
    }
    catch(std::exception const& e) { cerr << e.what() << endl;}
    catch(...) { cout << "Fail at " << p.string() << endl; }
    return false;
}

bool add_file(const bfs::path &p, int mtime)
{
    TagLib::FileRef f(p.string().c_str());
    if(!f.isNull() && f.tag()) {
        TagLib::Tag *tag = f.tag();
        int filesize = bfs::file_size(p);
        int bitrate = 0;
        int duration = 0;
        if(f.audioProperties()) {
            TagLib::AudioProperties *properties = f.audioProperties();
            duration = properties->length();
            bitrate = properties->bitrate();
        }
        string artist = tag->artist().to8Bit(true);
        string album  = tag->album().to8Bit(true);
        string track  = tag->title().to8Bit(true);
        boost::trim(artist);
        boost::trim(album);
        boost::trim(track);
        if(artist.length()==0 || track.length()==0) 
        {
            ignored++;
            return false;
        }
        string ext(bfs::extension(p));
        string mimetype = ext2mime(to_lower_copy(ext));
        gLibrary->add_file( p.string(), 
                            mtime, 
                            filesize, 
                            string("md5here"), 
                            mimetype,
                            duration,
                            bitrate,
                            artist, 
                            album, 
                            track, 
                            tag->track());

       /* cout << "-- TAG --" << endl;
        cout << "title   - \"" << tag->title()   << "\"" << endl;
        cout << "artist  - \"" << tag->artist()  << "\"" << endl;
        cout << "album   - \"" << tag->album()   << "\"" << endl;
        cout << "year    - \"" << tag->year()    << "\"" << endl;
        cout << "comment - \"" << tag->comment() << "\"" << endl;
        cout << "track   - \"" << tag->track()   << "\"" << endl;
        cout << "genre   - \"" << tag->genre()   << "\"" << endl;*/
        scanned++;
        return true;
    } else {
        ignored++;
        return false;
    }
}

string ext2mime(string ext)
{
    
    if(ext==".mp3") return "audio/mpeg";
    if(ext==".aac") return "audio/mp4";
    if(ext==".mp4") return "audio/mp4";
    if(ext==".m4a") return "audio/mp4"; 
    cerr << "Warning, unhandled file extension. Don't know mimetype for " << ext << endl;
    // generic:
    return "application/octet-stream";
}

int main(int argc, char *argv[])
{
    if(argc<3 || argc==1){
        cerr<<"Usage: "<<argv[0] << " <collection.db> <scan_dir>"<<endl;
        return 1;
    }
    gLibrary = new Library(argv[1], 0);
    // get last scan date:
    cout << "Loading data from last scan..." << flush;
    map<string,int> mtimes = gLibrary->file_mtimes();
    cout << mtimes.size() << " files+dir mtimes loaded" << endl;
    cout << "Scanning for changes..." << endl;
    bfs::path dir(argv[2]);
    sqlite3pp::transaction xct(*(gLibrary->db()));
    {
        // first scan for mp3/aac/etc files:
        try
        {
            scan(dir, mtimes);
            cout << "Scan complete ok." << endl;
        }
        catch(...)
        {
            cout << "Scan failed." << endl;
            xct.rollback();
            return 1;
        }
        // now create fuzzy text index:
        try
        {
            cout << endl << "Building search indexes..." << endl;
            gLibrary->build_index("artist");
            gLibrary->build_index("album");
            gLibrary->build_index("track");
            xct.commit();
            cout << "Finished,   scanned: " << scanned 
                << " skipped: " << skipped 
                << " ignored: " << ignored 
                << endl;
        }
        catch(...)
        {
            cout << "Index update failed, delete your database, fix the bug, and retry" << endl;
            xct.rollback();
            return 1;
        }
    }
    delete(gLibrary); 
    return 0;
}

