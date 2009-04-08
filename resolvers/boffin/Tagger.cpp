#include <fstream>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "BoffinDb.h"

using namespace std;

// [id]\tArtist\tAlbum\tTrack\n
bool track_out(int id, const string& artist, const string& album, const string& track)
{
    printf("%d\t%s\t%s\t%s\n", id, artist.data(), album.data(), track.data());
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
                // lexical_cast failures, try next line
            }
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    if(argc < 4) {
        cerr << "Usage: " << argv[0] << " [out|in] <playdar.db> <boffin.db>" << endl;
        return 1;
    }

    try {
        BoffinDb db(argv[3], argv[2]);
        
        if (stricmp(argv[1], "out") == 0) {
            db.map_files_without_tags(track_out);
        } else if (stricmp(argv[1], "in") == 0) {
            if (argc > 4) {
                ifstream file( argv[4] );
                db.update_tags( boost::bind( parse_line, boost::ref(file), _1, _2 ) );
            } else {
                db.update_tags( boost::bind( parse_line, boost::ref(cin), _1, _2 ) );
            }
        }
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