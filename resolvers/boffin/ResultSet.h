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


#ifndef RESULT_SET_H
#define RESULT_SET_H

#include <set>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

struct TrackResult
{
    TrackResult(int t, int a, float w)
        : trackId(t)
        , artistId(a)
        , weight(w)
    {
    }

    int trackId;
    int artistId;
    mutable float weight;

    bool operator<(const TrackResult& that) const
    {
        return this->trackId < that.trackId;
    }
};

class ResultSet : public std::set<TrackResult> 
{
public:
    void insertTrackResult(int track, int artist, float weight)
    {
        insert(TrackResult(track, artist, weight));
    }
};

typedef boost::shared_ptr<ResultSet> ResultSetPtr;


/// some set operation helper funcs:
namespace setop {

// c = a intersect b
template<typename T>
void and(const std::set<T>& a, const std::set<T>& b, std::set<T> &c)
{
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(c, c.begin()) );
}

// c = a subtract b
template<typename T>
void and_not(const std::set<T>& a, const std::set<T>& b, std::set<T> &c)
{
    std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::inserter(c, c.begin()) );
}

}

#endif
