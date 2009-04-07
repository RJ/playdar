#ifndef __BOFFIN_DB_H__
#define __BOFFIN_DB_H__

#include <map>
#include <vector>
#include <string>
#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#include "sqlite3pp.h"

class BoffinDb
{
public:
    typedef std::vector< boost::tuple<std::string, float, int> > TagCloudVec;   // tagname, total weight, track count
    typedef std::vector< std::pair<int, float> > TagVec;                        // tag_id, weight
    typedef std::map<int, TagVec> ArtistTagMap;

    BoffinDb(const std::string& boffinDbFilePath, const std::string& playdarDbFilePath);

    static std::string sortname(const std::string& name);

    boost::shared_ptr<TagCloudVec> get_tag_cloud();
    void get_all_artist_tags(ArtistTagMap& out);
    int get_tag_id(const std::string& tag);
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

    template<typename Functor>
    void update_tags(Functor parseResultLine)
    {
        sqlite3pp::transaction xct( m_db );
        {
            typedef std::pair<std::string, float> Tag;

            int fileId;
            std::vector<Tag> tags;
            while (parseResultLine( fileId, tags )) {
                BOOST_FOREACH(Tag& tag, tags) {
                    sqlite3pp::command cmd( m_db, "INSERT INTO file_tag (file, tag, weight) VALUES (?, ?, ?)" );
                    cmd.binder() << fileId;
                    int tagId = get_tag_id( tag.first );
                    cmd.binder() << tagId << tag.second;
                    cmd.execute();
                }
                tags.clear();
            }
        }
        xct.commit();
    }

private:
    sqlite3pp::database m_db;
};

#endif