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
#ifndef __BOFFIN_DB_H__
#define __BOFFIN_DB_H__

#include <map>
#include <vector>
#include <string>
#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#include "sqlite3pp.h"
#include <iostream>

class BoffinDb
{
public:
    // TagCloudVecItem tuple: tagname, total weight, track count, total time (seconds)
    typedef boost::tuple<std::string, float, int, int> TagCloudVecItem;              
    typedef std::vector<TagCloudVecItem> TagCloudVec;       
    typedef std::vector< std::pair<int, float> > TagVec;                        // tag_id, weight
    typedef std::map<int, TagVec> ArtistTagMap;

    enum CreateFlag { Create, NoCreate };

    BoffinDb(const std::string& boffinDbFilePath, const std::string& playdarDbFilePath);

    static std::string sortname(const std::string& name);
    boost::shared_ptr<TagCloudVec> get_tag_cloud(int limit = 0);
    void get_all_artist_tags(ArtistTagMap& out);
    int get_tag_id(const std::string& tag, CreateFlag create = Create );
    int get_artist_id(const std::string& artist);

    // for each track without tags, call f(track_id, artist_sortname, album_sortname, track_sortname)
    template<typename Functor>
    void map_files_without_tags(Functor f)
    {
        sqlite3pp::query qry(m_db, 
            "SELECT pd.file_join.track, pd.artist.sortname, pd.album.sortname, pd.track.sortname FROM pd.file_join "
            "INNER JOIN pd.artist ON pd.file_join.artist = pd.artist.id "
            "LEFT JOIN pd.album ON pd.file_join.album = pd.album.id "
            "INNER JOIN pd.track ON pd.file_join.track = pd.track.id "
            "WHERE pd.file_join.track NOT IN( SELECT track_tag.track FROM track_tag )");
        for(sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
            if (! f( i->get<int>(0), i->get<std::string>(1), i->get<std::string>(2), i->get<std::string>(3) ) ) {
                break;
            }
        }
    }

    // Functor is: bool getResultLine(int& trackId, std::vector<Tag>& tags)
    template<typename Functor>
    void update_tags(Functor getResultLine)
    {
        sqlite3pp::transaction xct( m_db );
        {
            typedef std::pair<std::string, float> Tag;

            int fileId;
            std::vector<Tag> tags;
            while (getResultLine( fileId, tags )) {
                BOOST_FOREACH(Tag& tag, tags) {
                    sqlite3pp::command cmd( m_db, "INSERT INTO track_tag (rowid, track, tag, weight) VALUES (null, ?, ?, ?)" );
                    cmd.bind(1, fileId);
                    cmd.bind(2, get_tag_id( tag.first ));
                    cmd.bind(3, tag.second);
                    cmd.execute();
                }
                tags.clear();
            }
        }
        xct.commit();
    }

    // Functor is: void onFile(int file_id, int artist_id, float weight)
    template<typename Functor>
    void files_with_tag(const std::string& tag, float minWeight, Functor onFile)
    {
        int tagId = get_tag_id(tag, NoCreate);
        if (tagId > 0) {
            sqlite3pp::query qry( m_db,
                "SELECT pd.file_join.file, artist, track_tag.weight FROM pd.file_join "
                "INNER JOIN track_tag ON pd.file_join.track = track_tag.track "
                "WHERE tag = ? AND weight > ?");
            
            qry.bind(1, tagId);
            qry.bind(2, minWeight);
            for(sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
                onFile( i->get<int>(0), i->get<int>(1), i->get<float>(2) );
            }
        }
    }

    // Functor is: void onFile(int track, int artist)
    template<typename Functor>
    int files_by_artist(const std::string& artist, Functor onFile)
    {
        int count = 0;
        sqlite3pp::query qry( m_db,
            "SELECT file, artist FROM pd.file_join "
            "INNER JOIN pd.artist ON pd.file_join.artist = pd.artist.id "
            "WHERE pd.artist.sortname = ?");
        qry.bind(1, sortname(artist).data());
        for(sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i, count++) {
            onFile( i->get<int>(0), i->get<int>(1) );
        }
        return count;
    }

    // Functor is: void onFile(int track, int artist)
    template<typename Functor>
    int files_by_artist(int artistId, Functor onFile)
    {
        int count = 0;
        sqlite3pp::query qry( m_db,
            "SELECT file, artist FROM pd.file_join "
            "WHERE artist = ?");
        qry.bind(1, artistId);
        for(sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i, count++) {
            onFile( i->get<int>(0), i->get<int>(1) );
        }
        return count;
    }
    
    sqlite3pp::database& db()
    {
        return m_db;
    }

private:
    void check_db();
    void create_db_schema();
    sqlite3pp::database m_db;
};

#endif
