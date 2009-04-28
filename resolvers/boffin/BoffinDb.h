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
    typedef boost::tuple<std::string, float, int> TagCloudVecItem;
    typedef std::vector<TagCloudVecItem> TagCloudVec;       // tagname, total weight, track count
    typedef std::vector< std::pair<int, float> > TagVec;                        // tag_id, weight
    typedef std::map<int, TagVec> ArtistTagMap;

    enum CreateFlag { Create, NoCreate };

    BoffinDb(const std::string& boffinDbFilePath, const std::string& playdarDbFilePath);

    static std::string sortname(const std::string& name);
    boost::shared_ptr<TagCloudVec> get_tag_cloud(int limit);
    void get_all_artist_tags(ArtistTagMap& out);
    int get_tag_id(const std::string& tag, CreateFlag create = Create );
    int get_artist_id(const std::string& artist);

    // for each file without tags, call f(fileid, artist, album, track)
    template<typename Functor>
    void map_files_without_tags(Functor f)
    {
        sqlite3pp::query qry(m_db, 
            "SELECT pd.file_join.file, pd.artist.sortname, pd.album.sortname, pd.track.sortname FROM pd.file_join "
            "INNER JOIN pd.artist ON pd.file_join.artist = pd.artist.id "
            "LEFT JOIN pd.album ON pd.file_join.album = pd.album.id "
            "INNER JOIN pd.track ON pd.file_join.track = pd.track.id "
            "WHERE pd.file_join.file NOT IN( SELECT file_tag.file FROM file_tag )");
        for(sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
            if (! f( i->get<int>(0), i->get<std::string>(1), i->get<std::string>(2), i->get<std::string>(3) ) ) {
                break;
            }
        }
    }

    // Functor is: bool getResultLine(int& fileId, std::vector<Tag>& tags)
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
                    sqlite3pp::command cmd( m_db, "INSERT INTO file_tag (rowid, file, tag, weight) VALUES (null, ?, ?, ?)" );
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

    // Functor is: void onFile(int track, int artist, float weight)
    template<typename Functor>
    void files_with_tag(const std::string& tag, Functor onFile)
    {
        int tagId = get_tag_id(tag, NoCreate);
        if (tagId > 0) {
            sqlite3pp::query qry( m_db,
                "SELECT file_tag.file, artist, file_tag.weight FROM pd.file_join "
                "INNER JOIN file_tag ON pd.file_join.file = file_tag.file "
                "WHERE tag = ?");
            
            qry.bind(1, tagId);
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
    
    sqlite3pp::database* db()
    {
        return &m_db;
    }

private:
    void check_db();
    void create_db_schema();
    sqlite3pp::database m_db;
};

#endif
