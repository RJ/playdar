#ifndef __BOFFIN_DB_H__
#define __BOFFIN_DB_H__

#include <map>
#include <vector>
#include <string>
#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>

#include "sqlite3pp.h"

class BoffinDb
{
public:
    typedef std::vector< boost::tuple<std::string, float, int> > TagCloudVec;   // tagname, total weight, track count
    typedef std::vector< std::pair<int, float> > TagVec;                        // tag_id, weight
    typedef std::map<int, TagVec> ArtistTagMap;

    BoffinDb(const std::string& boffinDbFilePath, const std::string& playdarDbFilePath);
    boost::shared_ptr<TagCloudVec> get_tag_cloud();
    void get_all_artist_tags(ArtistTagMap& out);

    //
    int get_artist_id(const std::string& artist);

private:
    sqlite3pp::database m_db;
};

#endif