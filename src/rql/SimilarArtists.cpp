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

// Returns a TagDataset::Entry containing tags for the artist.
// Calls the last.fm webservice to get the tags and blocks
// for up to 10 seconds while waiting for the response.
// 
// coll is used to look-up/create tag ids from tag names
// artistId is used to initialise the artistId member of the result
// artist is the name sent to the last.fm web service
//
// a return containing an empty tagVec could be considered failure
//
TagDataset::Entry
dlArtistTags(Library& library, const string& artist, int artistId)
{
    TagDataset::Entry result;
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


void
resultCb(list<SimilarArtists::Result>& results, const TagDataset::Entry& entry, float score)
{
    if (score > 0.1) {
        results.push_back(make_pair(entry.artistId, score));
    }
}

list<SimilarArtists::Result>
SimilarArtists::getSimilarArtists(Library& library, const QString& artist, int artistId)
{
    list<Result> results;

    m_dataset.load(coll);

    TagDataset::Entry otherArtist;
    TagDataset::Entry *pArtist = artistId > 0 ? m_dataset.findArtist(artistId) : 0;

    if (pArtist == 0) {
        otherArtist = dlArtistTags(coll, artist, artistId);
        if (otherArtist.tagVec.size()) {
            // result looks good
            pArtist = &otherArtist;
            if (artistId < 1) {
                // this previously-unknown-to-us artist has some tags 
                // at Last.fm, so why not add their name to our db:
                otherArtist.artistId = library.getArtistId(artist, LocalCollection::Create);
            }
        }
    }

    if (pArtist && m_dataset.m_allArtists.size()) {
        if (m_dataset.m_allArtists.size()) {
            moost::algo::CosSimilarity::findSimilar/*<TagDataset::Entry, TagDataset::EntryList, TagDataset>*/(
                boost::bind(resultCb, boost::ref(results), _1, _2),
                *pArtist,
                m_dataset.m_allArtists,
                m_dataset);
        }
    }

    return results;
}


bool 
artistList_orderByWeightDesc(const SimilarArtists::Result& a, const SimilarArtists::Result& b)
{
    return a.second > b.second;
}

void
SimilarArtists::buildArtistFilter(Library& library, int artistId, set<int>& filterSet)
{
    // These artists are too random, so we exclude them from sim-art.
    // Maybe when track tags are actually track tags (and not artist 
    // tags) we won't need to do this.  Maybe?
    const char* duds[] = { "various artists", "various", "soundtrack", "[unknown]", NULL };
    for (const char** artist = &duds[0]; *artist; artist++) {
        int id = coll.getArtistId(*artist, LocalCollection::NoCreate);
        if (id > 0) filterSet.insert(id);
    }

    if (result.contains(artistId)) {
        // unless we're looking for the similar artists of 
        // "various" etc, then the above are good candidates!
        filterSet.clear();
    }

    // the artist of "similar artist" is always excluded:
    filterSet += artistId;
}

ResultSetPtr
SimilarArtists::filesBySimilarArtist(Library& library, const char* artist)
{
    ResultSet result;

    QString qsArtist = QString(artist).simplified().toLower();
    int artistId = library.getArtistId(qsArtist, LocalCollection::NoCreate);

    set<int> artistFilter;
    buildArtistFilter(library, artistId, artistFilter);

    list<SimilarArtists::Result> artistList = getSimilarArtists(coll, qsArtist, artistId);
    qSort(artistList.begin(), artistList.end(), artistList_orderByWeightDesc);

    list<SimilarArtists::Result>::const_iterator pArtist = artistList.begin();
    int artistCount = 0;
    int trackCount = 0;
    while ((trackCount < 10000 || artistCount < 20) && pArtist != artistList.end())
    {
        if (!artistFilter.contains(pArtist->first)) {
            QList<uint> tracks = coll.filesByArtistId(pArtist->first, LocalCollection::AvailableSources);
            foreach(uint trackId, tracks) {
                result << TrackResult(trackId, pArtist->first, pArtist->second);
            }
            trackCount += tracks.size();
            artistCount++;
        }
        pArtist++;
    }

    return result;
}
