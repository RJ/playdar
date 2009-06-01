/*
    Playdar - music content resolver
    Copyright (C) 2009  Last.fm Ltd.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <fstream>
#include <sstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp> 
#include <curl/curl.h>
#include "BoffinDb.h"
#include "CurlChecker.hpp"

#define MUSICLOOKUP_URL "http://musiclookup.last.fm/trackresolve"

using namespace std;

// originally used stringstream instead of the ostringstream/istringstream pair
// but couldn't make it work reliably on all platforms! so here's an 
// uglier, less-efficient (hopefully) working attempt  :(
ostringstream ossreq;
istringstream* issreq;

ostringstream ossresp;
istringstream *issresp;



// [id]\tArtist\tAlbum\tTrack\n
bool track_out(int id, const string& artist, const string& album, const string& track)
{
    ossreq 
        << id << "\t"
        << artist << "\t"
        << album << "\t"
        << track << "\n";
    return true;
}

// [id]\tTag\tWeight...\n
bool parse_line(int& id, std::vector <std::pair<std::string, float> >& tags)
{
    while (issresp->good()) {
        string line;
        getline( *issresp, line );

        vector<string> splitVec;
        boost::split(splitVec, line, boost::is_any_of("\t"));
        
        if (splitVec.size() >= 3 && (splitVec.size() & 1)) {
            try {
                id = boost::lexical_cast<int>( splitVec[0] );
                for (size_t i = 1; i < splitVec.size(); i += 2)
                    tags.push_back(
                        std::make_pair(
                            splitVec[i],
                            boost::lexical_cast<float>( splitVec[i+1] ) ) );
                return true;
            } catch(...) {
                cerr << "dodgy line warning: [" << line << "]\n";
            }
        }
    }
    return false;
}

// curl has response data for us:
size_t curl_writer(void *ptr, size_t size, size_t nmemb, void *)
{
    size_t bytes = size * nmemb;
    ossresp.write((char*) ptr, bytes);
    return bytes;
}

// curl wants request data from us:
size_t curl_reader(void *ptr, size_t size, size_t nmemb, void *)
{
    return issreq->readsome((char*) ptr, size * nmemb);
}


int main(int argc, char *argv[])
{
    if(argc < 3) {
        cerr << "Usage: " << argv[0] << " <collection.db> <boffin.db>" << endl;
        return 1;
    }

    try {
        BoffinDb db(argv[2], argv[1]);
            
        // build up the request:
        db.map_files_without_tags(track_out);
        ossreq.flush();
        issreq = new istringstream(ossreq.str());

        // setup curl:
        CURL* curl = curl_easy_init();
        CurlChecker cc;

        // io options:
        cc("WRITEFUNCTION") = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writer);  
        cc("READFUNCTION") = curl_easy_setopt(curl, CURLOPT_READFUNCTION, curl_reader);

        // http options:
        cc("URL") = curl_easy_setopt(curl, CURLOPT_URL, MUSICLOOKUP_URL);  
        cc("HEADER") = curl_easy_setopt(curl, CURLOPT_HEADER, 0);                           // don't include header in response

        cc("FOLLOWLOCATION") = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);           // follow http redirects
        // leave this to the defaults with older versions of curl
#if defined(CURLOPT_POSTREDIR) && defined(CURL_REDIR_POST_ALL)
        cc("POSTREDIR") = curl_easy_setopt(curl, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL );  // keep on posting when redir'd (301 or 302)
#endif
        cc("POST") = curl_easy_setopt(curl, CURLOPT_POST, 1);
        cc("POSTFIELDSIZE") = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, issreq->rdbuf()->in_avail());
        cc("USERAGENT") = curl_easy_setopt(curl, CURLOPT_USERAGENT, "playdar boffin tagger");
        struct curl_slist *headers = 0;
        headers = curl_slist_append(headers, "content-type: text/plain; charset=utf-8");
        headers = curl_slist_append(headers, "expect:");
        cc("HTTPHEADER") = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        cc("easy_perform") = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        ossresp.flush();
        issresp = new istringstream(ossresp.str());
        db.update_tags( parse_line );
    } 
    catch (const std::exception& e) {
        cerr << "failed: " << e.what();
        return 1;
    } 
    //catch (...) {
    //    cerr << "failed with unhandled exception";
    //    return 1;
    //}

}
