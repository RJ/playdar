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

stringstream ssreq;
stringstream ssresp;

// [id]\tArtist\tAlbum\tTrack\n
bool track_out(int id, const string& artist, const string& album, const string& track)
{
    ssreq 
        << id << "\t"
        << artist << "\t"
        << album << "\t"
        << track << "\n";
    return true;
}

// [id]\tTag\tWeight...\n
bool parse_line(istream& in, int& id, std::vector <std::pair<std::string, float> >& tags)
{
    while (in.good()) {
        string line;
        getline( in, line );

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
    ssresp.write((char*) ptr, bytes);
    return bytes;
}

// curl wants request data from us:
size_t curl_reader(void *ptr, size_t size, size_t nmemb, void *)
{
    return ssreq.readsome((char*) ptr, size * nmemb);
}


int main(int argc, char *argv[])
{
    if(argc < 3) {
        cerr << "Usage: " << argv[0] << " <playdar.db> <boffin.db>" << endl;
        return 1;
    }

    try {
        BoffinDb db(argv[2], argv[1]);
            
        // build up the request:
        db.map_files_without_tags(track_out);

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
        cc("POSTREDIR") = curl_easy_setopt(curl, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL );  // keep on posting when redir'd (301 or 302)
        cc("POST") = curl_easy_setopt(curl, CURLOPT_POST, 1);
        cc("POSTFIELDSIZE") = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, ssreq.rdbuf()->in_avail());
        cc("USERAGENT") = curl_easy_setopt(curl, CURLOPT_USERAGENT, "playdar boffin tagger");
        struct curl_slist *headers = curl_slist_append(0, "text/plain; charset=utf-8");
        cc("HTTPHEADER") = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        cc("easy_perform") = curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        db.update_tags( boost::bind(parse_line, boost::ref(ssresp), _1, _2) );
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
