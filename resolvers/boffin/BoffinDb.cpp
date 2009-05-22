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
#include "BoffinDb.h"
#include "boffin_sql.h"

#include <string>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace std;

BoffinDb::BoffinDb(const std::string& boffinDbFilePath, const std::string& playdarDbFilePath)
: m_db( boffinDbFilePath.c_str() )
{
    // confirm DB is correct version, or create schema if first run
    check_db();
    m_db.attach( playdarDbFilePath.c_str(), "pd" );     // pd = playdar :)
}

void
BoffinDb::check_db()
{
    try
    {
      sqlite3pp::query qry(m_db, "SELECT value FROM boffin_system WHERE key = 'schema_version'");
      sqlite3pp::query::iterator i = qry.begin();
      if( i == qry.end() )
      {
        // unusual - table exists but doesn't contain this row.
        // could have been created wrongly
        cerr << "Error, boffin_system table missing schema_version key!" << endl
             << "Maybe you created the database wrong, or it's corrupt." << endl
             << "Try deleting it and re-scanning?" << endl;
        throw; // not caught here.
      }
      string val = (*i).get<string>(0);
      cout << "Boffin database schema detected as version " << val << endl;
      // check the schema version is what we expect
      // TODO auto-upgrade to newest schema version as needed.
      if( val != "1" )
      {
        cerr << "Boffin schema version too old. TODO handle auto-upgrades" << endl;
        throw; // not caught here
      }
      // OK.
    }
    catch(sqlite3pp::database_error err)
    {
      // probably doesn't exist yet, try and create it
      // 
      cout << "database_error: " << err.what() << endl;
      create_db_schema();
    }
    
}

void
BoffinDb::create_db_schema()
{
    cout << "Attempting to create boffin DB schema..." << endl;
    string sql(  playdar::get_boffin_sql() );
    vector<string> statements;
    boost::split( statements, sql, boost::is_any_of(";") );
    BOOST_FOREACH( string s, statements )
    {
        boost::trim( s );
        if(s.empty()) continue;
        cout << "Executing: " << s << endl;
        sqlite3pp::command cmd(m_db, s.c_str());
        cmd.execute();
    }
    cout << "Schema created." << endl;
}

// limit == 0 means no limits!
boost::shared_ptr<BoffinDb::TagCloudVec> 
BoffinDb::get_tag_cloud(int limit /* = 0 */)
{
    sqlite3pp::query qry(m_db, 
        limit <= 0 ?
        "SELECT name, sum(weight), count(weight) "
        "FROM track_tag "
        "INNER JOIN tag ON track_tag.tag = tag.rowid "
        "GROUP BY tag.rowid"
        :
        "SELECT name, sum(weight), count(weight) "
        "FROM track_tag "
        "INNER JOIN tag ON track_tag.tag = tag.rowid "
        "GROUP BY tag.rowid "
        "LIMIT ?");
    if (limit) {
        qry.bind(1, limit);
    }

    boost::shared_ptr<TagCloudVec> p( new TagCloudVec() );
    float maxWeight = 0;
    for(sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
        p->push_back( i->get_columns<string, float, int>(0, 1, 2) );
        maxWeight = max( maxWeight, p->back().get<1>() );
    }
    
    //if (maxWeight > 0) {
    //    for( TagCloudVec::iterator i = p->begin(); i != p->end(); ++i ) {
    //        i->get<1>() = i->get<1>() / maxWeight;
    //    }
    //}
    
    return p;
}

void
BoffinDb::get_all_artist_tags(BoffinDb::ArtistTagMap& out)
{
    sqlite3pp::query qry(m_db, 
        "SELECT artist, tag, avg(weight) "
        "FROM track_tag "
        "INNER JOIN pd.file_join ON track_tag.file = pd.file_join.track "
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