#include "BoffinDb.h"

#include <string>
#include <boost/algorithm/string/trim.hpp>

using namespace std;

BoffinDb::BoffinDb(const std::string& boffinDbFilePath, const std::string& playdarDbFilePath)
: m_db( boffinDbFilePath.c_str() )
{
    m_db.attach( playdarDbFilePath.c_str(), "pd" );     // pd = playdar :)
}

boost::shared_ptr<BoffinDb::TagCloudVec> 
BoffinDb::get_tag_cloud(int limit)
{
    sqlite3pp::query qry(m_db, 
        "SELECT name, sum(weight), count(weight) "
        "FROM file_tag "
        "INNER JOIN tag ON file_tag.tag = tag.rowid "
        "GROUP BY tag.rowid");

    boost::shared_ptr<TagCloudVec> p( new TagCloudVec() );
    float maxWeight = 0;
    for(sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
        p->push_back( i->get_columns<string, float, int>(0, 1, 2) );
        maxWeight = max( maxWeight, p->back().get<1>() );
    }
    
    if (maxWeight > 0) {
        for( TagCloudVec::iterator i = p->begin(); i != p->end(); ++i ) {
            i->get<1>() = i->get<1>() / maxWeight;
        }
    }
    
    return p;
}

void
BoffinDb::get_all_artist_tags(BoffinDb::ArtistTagMap& out)
{
    sqlite3pp::query qry(m_db, 
        "SELECT pd.track.artist, tag, avg(weight) "
        "FROM file_tag "
        "INNER JOIN pd.file on file_tag.file = pd.track.id "
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

// get the id of the tag
// optionally create the tag if it doesn't exist
// returns tag id > 0 on success
int
BoffinDb::get_tag_id(const std::string& tag, BoffinDb::CreateFlag create /* = BoffinDb::Create */)
{
    std::string tag_sortname(sortname(tag));

    sqlite3pp::query qry(m_db, "SELECT rowid FROM tag WHERE name = ?");
    qry.bind(1, tag_sortname.data());
    sqlite3pp::query::iterator it = qry.begin();
    if (it != qry.end())
        return it->get<int>(0);

    if (create == BoffinDb::Create) {
        sqlite3pp::command cmd(m_db, "INSERT INTO tag (name) VALUES (?)");
        cmd.bind(1, tag_sortname.data());
        int result = cmd.execute();
        if (SQLITE_OK == result)
            return (int) m_db.last_insert_rowid();
    }
    return 0;
}

// get the id of the artist
// returns 0 if the artist doesn't exist
int 
BoffinDb::get_artist_id(const std::string& artist)
{
    std::string artist_sortname(sortname(artist));

    {
        sqlite3pp::query qry(m_db, "SELECT id FROM pd.artist WHERE sortname = ?");
        qry.bind(1, artist_sortname.data());
        sqlite3pp::query::iterator it = qry.begin();
        if (it != qry.end()) {
            return it->get<int>(0);
        }
    }

    return 0;
}

string
BoffinDb::sortname(const string& name)
{
    string data(name);
    std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    boost::trim(data);
    return data;
}