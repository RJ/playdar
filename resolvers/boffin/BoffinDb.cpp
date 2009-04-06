#include "BoffinDb.h"

#include <string>

using namespace std;

BoffinDb::BoffinDb(const std::string& boffinDbFilePath, const std::string& playdarDbFilePath)
: m_db( boffinDbFilePath.c_str() )
{
    m_db.attach( playdarDbFilePath.c_str(), "playdar" );
}

boost::shared_ptr<BoffinDb::TagCloudVec> 
BoffinDb::get_tag_cloud()
{
    sqlite3pp::query qry(m_db, 
        "SELECT name, sum(weight), count(weight) FROM track_tag"
        "INNER JOIN tag ON track_tag.tag = tag.id");
    int count = qry.begin()->data_count();
    boost::shared_ptr<TagCloudVec> p( new TagCloudVec( count ) );
    for(sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
        p->push_back( i->get_columns<string, float, int>(0, 1, 2) );
    }
    return p;
}

void
BoffinDb::get_all_artist_tags(BoffinDb::ArtistTagMap& out)
{
    sqlite3pp::query qry(m_db, 
        "SELECT artist, tag, avg(weight) "
        "FROM track_tag "
        "INNER JOIN files on track_tag.track = track.id "
        "GROUP BY artist, tag "
        "ORDER BY artist, tag ");

    {
        int prevArtistId = 0;
        TagVec currentArtistTags;

        for ( sqlite3pp::query::iterator it = qry.begin(); it != qry.end(); it++) {
            const int artistId = it->get<int>(0);
            const int tag = it->get<int>(1);
            const float weight = it->get<float>(2);

            if (prevArtistId == 0) // first run through the loop
                prevArtistId = artistId;

            if (prevArtistId != artistId) {
                out[prevArtistId] = currentArtistTags;
                currentArtistTags = TagVec();
                prevArtistId = artistId;
            }

            currentArtistTags.push_back( make_pair(tag, weight) );
        }

        if (currentArtistTags.size()) {
            out[prevArtistId] = currentArtistTags;
        }
    }
}

int 
BoffinDb::get_artist_id(const std::string& artist)
{
    // todo!
    return 0;
}