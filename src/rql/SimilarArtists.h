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

#ifndef SIMILAR_ARTISTS_H
#define SIMILAR_ARTISTS_H

#include "playdar/library.h"
#include "ResultSet.h"
#include <math.h>

struct TagDataset
{
    Library::ArtistTagMap m_allArtists;
    std::map<int, float> m_norms;

    static float calcNormal(const Library::TagVec& vec)
    {
        float sum = 0;
        for (Library::TagVec::const_iterator p = vec.begin(); p != vec.end(); p++) {
            sum += (p->second * p->second);
        }
        return sqrt(sum); 
    }

    bool load(Library& library)
    {
        // if data hasn't been loaded yet: load and
        // precalculate the normals for each artist

        if (0 == m_allArtists.size()) {
            library.get_all_artist_tags(m_allArtists);
            for(Library::ArtistTagMap::iterator it = m_allArtists.begin(); it != m_allArtists.end(); it++) {
                m_norms[it->first] = calcNormal(it->second);
            }
        }
        return true;
    }

    Library::ArtistTagMap::const_iterator findArtist(int artistId, bool *bFound)
    {
        Library::ArtistTagMap::iterator it = m_allArtists.find(artistId);
        *bFound = it != m_allArtists.end();
        return it;
    }

    //////////////////////////////////////////////////
    // these methods fulfil the policy requirements 
    // of the findSimilar template function

    static Library::TagVec::const_iterator get_begin(const Library::ArtistTagMap::const_iterator& it)
    {
        return it->second.begin(); 
    }

    static Library::TagVec::const_iterator get_end(const Library::ArtistTagMap::const_iterator& it)
    { 
        return it->second.end(); 
    }

    static int get_id(const Library::TagVec::const_iterator& it)
    { 
        return it->first; 
    }

    static float get_score(const Library::TagVec::const_iterator& it)
    { 
        return it->second; 
    }

    float get_norm(const Library::ArtistTagMap::const_iterator& it) const
    {
//        ASSERT(m_norms.find(it->first) != m_norms.end());
        // but really, m_norms.find will always succeed!
        return m_norms.find(it->first)->second;
    }

    template<typename Dummy>
    double apply_post_process( Dummy, Dummy, const std::pair<double, int>& simVal) const
    {
        if ( simVal.second < 5 )
            return simVal.first * ( static_cast<double>(simVal.second) / 5 );
        else
            return simVal.first;
    }

};


class SimilarArtists
{
    TagDataset m_dataset;

public:
    ResultSetPtr filesBySimilarArtist(Library& library, const char *artist);

private:
    typedef std::pair<int, float> SimilarArtist;
    static void resultCb(std::list<SimilarArtist>& results, const Library::ArtistTagMap::const_iterator& it, float score);
    static bool artistList_orderByWeightDesc(const SimilarArtist& a, const SimilarArtist& b);
    void getSimilarArtists(Library& library, const std::string& artist, int artistId, std::list<SimilarArtist>& out);

    // for dealing with va, soundtrack, unknown, etc.
    static void buildArtistFilter(Library& library, int artistId, std::set<int>& out);
};


#endif