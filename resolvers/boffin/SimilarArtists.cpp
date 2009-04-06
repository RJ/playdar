/***************************************************************************
 *   Copyright 2007-2009 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "SimilarArtists.h"
//#include "lib/lastfm/types/Tag.h"
//#include "lib/lastfm/types/Artist.h"
//#include "lib/lastfm/ws/WsReplyBlock.h"
#include "similarity/CosSimilarity.h"
#include <boost/bind.hpp>
#include <list>

using namespace std;

// Returns a Library::ArtistTags containing tags for the artist.
// Calls the last.fm webservice to get the tags and blocks
// for up to 10 seconds while waiting for the response.
// 
// coll is used to look-up/create tag ids from tag names
// artistId is used to initialise the artistId member of the result
// artist is the name sent to the last.fm web service
//
// a return containing an empty tagVec could be considered failure
//
Library::TagVec
dlArtistTags(Library& library, const string& artist, int artistId)
{
    Library::TagVec result;
#if 0 // TODO!
    result.artistId = artistId;

    WsReply* reply = WsReplyBlock::wait(
        Artist(artist).getTopTags(),
        10 * 1000);     // 10 seconds

    if (reply) {
        // parse result and sort higher weights to top
        QMap<int, QString> tags = Tag::list(reply);

        // take the top ten tags having non-zero weight
        QMapIterator<int, QString> i( tags );
        i.toBack();
        for (int x = 0; x < 10; ++x) {
            if (i.hasPrevious() == false) break;
            if (i.previous().key() <= 0) break;
            TagDataset::TagId tagId = coll.getTagId(i.value(), LocalCollection::Create);
            // webservice gives weight 0..100; we want it 0..1
            TagDataset::TagWeight tagWeight = i.key() / 100.0;
            result.tagVec.push_back( qMakePair(tagId, tagWeight) );
        }
        result.norm = TagDataset::calcNormal(result.tagVec);
    }
#endif

    return result;
}

//////////////////////////////////////


//static
void
SimilarArtists::resultCb(list<SimilarArtists::SimilarArtist>& results, const Library::ArtistTagMap::const_iterator& it, float score)
{
    if (score > 0.1) {
        results.push_back(make_pair(it->first, score));
    }
}

void
SimilarArtists::getSimilarArtists(Library& library, const string& artist, int artistId, list<SimilarArtists::SimilarArtist>& out)
{
    m_dataset.load(library);

    bool found = false;
    Library::ArtistTagMap::const_iterator pArtist = m_dataset.findArtist(artistId, &found);

    if (!found) {
        // todo
        return;
        //otherArtist = dlArtistTags(library, artist, artistId);
        //if (otherArtist.size()) {
        //    // result looks good
        //    pArtist = &otherArtist;
        //}
    }

    moost::algo::CosSimilarity::findSimilar(
        boost::bind(resultCb, boost::ref(out), _1, _2),
        pArtist,
        m_dataset.m_allArtists,
        m_dataset);
}

//static
bool 
SimilarArtists::artistList_orderByWeightDesc(const SimilarArtist& a, const SimilarArtist& b)
{
    return a.second > b.second;
}

void
SimilarArtists::buildArtistFilter(Library& library, int artistId, set<int>& out)
{
    // These artists are too random, so we exclude them from sim-art.
    // Maybe when track tags are actually track tags (and not artist 
    // tags) we won't need to do this.  Maybe?
    static const char* duds[] = { "various artists", "various", "soundtrack", "[unknown]", NULL };
    for (const char** artist = &duds[0]; *artist; artist++) {
        int id = library.get_artist_id(*artist);
        if (id > 0) out.insert(id);
    }

    if (out.find(artistId) != out.end()) {
        // unless we're looking for the similar artists of 
        // "various" etc, then the above are good candidates!
        out.clear();
    }

    // the artist of "similar artist" is always excluded:
    out.insert(artistId);
}

ResultSetPtr
SimilarArtists::filesBySimilarArtist(Library& library, const char* artist)
{
    ResultSetPtr result( new ResultSet() );

    int artistId = library.get_artist_id(artist);

    set<int> artistFilter;
    buildArtistFilter(library, artistId, artistFilter);

    list<SimilarArtist> artistList;
    getSimilarArtists(library, artist, artistId, artistList);
    artistList.sort( artistList_orderByWeightDesc );

    list<SimilarArtist>::const_iterator pArtist = artistList.begin();
    int artistCount = 0;
    int trackCount = 0;
    while ((trackCount < 10000 || artistCount < 20) && pArtist != artistList.end())
    {
        if (artistFilter.find(pArtist->first) == artistFilter.end()) 
        {
            // artist is not in the filter set.

            list<int> tracks;
            //library.tracksByArtistId(pArtist->first, tracks);
            //foreach(uint trackId, tracks) {
            //    result << TrackResult(trackId, pArtist->first, pArtist->second);
            //}
            trackCount += tracks.size();
            artistCount++;
        }
        pArtist++;
    }

    return result;
}
