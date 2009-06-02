#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <sqlite3.h>

#include "../library.h"

#include "playdar/utils/urlencoding.hpp"

#include <iostream>
#include <cstdio>

using namespace std;
using namespace boost;
using namespace playdar;

namespace bfs = boost::filesystem; 


#ifdef WIN32

// to handle unicode file/pathnames on windows, we use wpath

typedef bfs::wpath Path;
typedef bfs::wdirectory_iterator DirIt;
wstring fromUtf8(const string& i);
string toUtf8(const wstring& i);

#else

// things are easier

typedef bfs::path Path;
typedef bfs::directory_iterator DirIt;
#define fromUtf8(s) (s)
#define toUtf8(s) (s)

#endif


bool add_file(const Path&, int mtime);
bool add_dir(const Path&);
string ext2mime(const string& ext);

Library *gLibrary;

int scanned, skipped, ignored = 0;

string url_encode( const string& p )
{
    // url encode everything between '/' character
    vector<std::string> v;
    split(v, p, boost::is_any_of( "/" ));
    string ret = "";
    bool first = true;
    BOOST_FOREACH(const string& s, v) {
        if (first) {
            first = false;
        } else {
            ret += "/";
        }
        ret += playdar::utils::url_encode(s);
    }
    return ret;
}

string urlify(const string& p)
{
    // turn it into a url by prepending file://
    // because we pass all urls to curl:
    string urlpath("file://");
    if (p.at(0)=='/') // posix path starting with /
    {
        urlpath += url_encode( p );
    }
    else if (p.at(1)==':') // windows style filepath
    {
        urlpath += "/";
        urlpath += p.substr(0, 2) + url_encode( p.substr(2) );  // encode the part after the drive-letter
    }
    else
    {
        // could be anything, hopefully something curl understands:
        // (presume that it's already encoded as we don't want to 
        //  encode the protocol part of the url )
        urlpath = p;
    }
    return urlpath;
}

bool scan(const Path& p, map<string, int>& mtimes)
{
    try
    {
        if(!exists(p)) 
            return false;

        DirIt end_itr;
        for(DirIt itr( p ); itr != end_itr; ++itr){
            if ( bfs::is_directory( itr->status() ) ){
                cout << "DIR: " << toUtf8(itr->path().string()) << endl;
                scan(itr->path(), mtimes);
            } else {
                // is this file an audio file we understand?
                string extu(toUtf8(bfs::extension(itr->path())));
                string ext = to_lower_copy(extu);
                if( ext == ".mp3" ||
                    ext == ".m4a" || 
                    ext == ".mp4" ||  
                    ext == ".aac" )
                {
                    string url = urlify( toUtf8(itr->string()) );
                    int mtime = bfs::last_write_time(itr->path());

                    map<string, int>::iterator mtimeit = mtimes.find(url);
                    if (mtimeit == mtimes.end() // not scanned previously
                        || mtimes[url] != mtime) // modified since last time
                    {
                        add_file(itr->path(), mtime);
                    } else {
                        ignored++;
                    }
                } else {
                    ignored++;
                    cout << "Ignoring: " << itr->string() << endl;
                }
            }
        }
    }
    catch(std::exception const& e) { cerr << e.what() << endl;}
    //catch(...) { cout << "Fail at " << p.string() << endl; }

    return true;
}

bool add_dir(const Path &p)
{
    try
    {
        int mtime = bfs::last_write_time(p);
        gLibrary->add_dir(toUtf8(p.string()), mtime);
        return true;
    }
    catch(const std::exception& e) { 
        cerr << e.what() << endl;
    }
    catch(...) { 
        cerr << "Fail at " << p.string() << endl; 
    }
    return false;
}

bool add_file(const Path& p, int mtime)
{
    TagLib::FileRef f(p.string().c_str());
    if (!f.isNull() && f.tag()) {
        TagLib::Tag *tag = f.tag();
        int filesize = bfs::file_size(p);
        int bitrate = 0;
        int duration = 0;
        if (f.audioProperties()) {
            TagLib::AudioProperties *properties = f.audioProperties();
            duration = properties->length();
            bitrate = properties->bitrate();
        }
        string artist = tag->artist().toCString(true);
        string album  = tag->album().toCString(true);
        string track  = tag->title().toCString(true);
        boost::trim(artist);
        boost::trim(album);
        boost::trim(track);
        if (artist.length()==0 || track.length()==0) {
            ignored++;
            return false;
        }

        string ext(toUtf8(bfs::extension(p)));
        string mimetype = ext2mime(to_lower_copy(ext));
        
        // turn it into a url by prepending file://
        // because we pass all urls to curl:
        string urlpath = urlify( toUtf8(p.string()) );
        
        gLibrary->add_file( urlpath, 
                            mtime, 
                            filesize, 
                            string(""), //TODO file hash?
                            mimetype,
                            duration,
                            bitrate,
                            artist, 
                            album, 
                            track, 
                            tag->track() );

       /* cout << "-- TAG --" << endl;
        cout << "title   - \"" << tag->title()   << "\"" << endl;
        cout << "artist  - \"" << tag->artist()  << "\"" << endl;
        cout << "album   - \"" << tag->album()   << "\"" << endl;
        cout << "year    - \"" << tag->year()    << "\"" << endl;
        cout << "comment - \"" << tag->comment() << "\"" << endl;
        cout << "track   - \"" << tag->track()   << "\"" << endl;
        cout << "genre   - \"" << tag->genre()   << "\"" << endl;*/
        cout << "T " 
             << tag->artist() << "\t"
             << tag->album() << "\t"
             << tag->title() << "\t" << endl;

        scanned++;
        return true;
    } else {
        ignored++;
        return false;
    }
}

string ext2mime(const string& ext)
{ 
    if(ext==".mp3") return "audio/mpeg";
    if(ext==".aac") return "audio/mp4";
    if(ext==".mp4") return "audio/mp4";
    if(ext==".m4a") return "audio/mp4"; 
    cerr << "Warning, unhandled file extension. Don't know mimetype for " << ext << endl;
     //generic:
    return "application/octet-stream";
}

#ifdef WIN32
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    if (argc<3 || argc==1) {
        cerr<<"Usage: "<< toUtf8(argv[0]) << " <collection.db> <scan_dir>"<<endl;
        return 1;
    }
    try {
        gLibrary = new Library(toUtf8(argv[1]));

        // get last scan date:
        cout << "Loading data from last scan..." << flush;
        map<string, int> mtimes = gLibrary->file_mtimes();
        cout << "" << mtimes.size() << " files+dir mtimes loaded" << endl;
        cout << "Scanning for changes..." << endl;
        Path dir(argv[2]);
        sqlite3pp::transaction xct(gLibrary->db());
        {
            // first scan for mp3/aac/etc files:
            try
            {
                scan(dir, mtimes);
                cout << "Scan complete ok." << endl;
            }
            catch(...)
            {
                cerr << "Scan failed." << endl;
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
                cerr << "Index update failed, delete your database, fix the bug, and retry" << endl;
                xct.rollback();
                return 1;
            }
        }
        delete gLibrary; 
    } catch (const std::exception& e) {
        cerr << "failed: " << e.what();
        return 1;
    } catch (...) {
        cerr << "failed with unhandled exception";
        return 1;
    }
    return 0;
}


#ifdef WIN32

wstring fromUtf8(const string& s)
{
    wstring r;
    const int len = s.length();
    if (len) {
        int chars = MultiByteToWideChar(CP_UTF8, 0, s.data(), len, NULL, 0);
        if (!chars) throw std::runtime_error("fixme: string conversion error");
        wchar_t* buffer = new wchar_t[chars];

        int result = MultiByteToWideChar(CP_UTF8, 0, s.data(), len, buffer, chars);
        if (result > 0) r.assign(buffer, result);
        delete [] buffer;
        if (result == 0) throw std::runtime_error("fixme: string conversion error");
    }
    return r;
}

string toUtf8(const wstring& s)
{
    string r;
    const int len = s.length();
    if (len) {
        int bytes = WideCharToMultiByte(CP_UTF8, 0, s.data(), len, NULL, 0, NULL, NULL);
        if (!bytes) throw std::runtime_error("fixme: string conversion error");
        char* buffer = new char[bytes];

        int result = WideCharToMultiByte(CP_UTF8, 0, s.data(), len, buffer, bytes, NULL, NULL);
        if (result > 0) r.assign(buffer, result);
        delete [] buffer;
        if (result == 0) throw std::runtime_error("fixme: string conversion error");
    }
    return r;
}

#endif
