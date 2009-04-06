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
    float weight;

    bool operator<(const TrackResult& that) const
    {
        return this->trackId < that.trackId;
    }
};

typedef std::set<TrackResult> ResultSet;
typedef boost::shared_ptr<ResultSet> ResultSetPtr;


/// some set operation helper funcs:
// todo: can further generalise template types
namespace setop {

// returns true when item is not in set
template<typename T> 
bool itemNotIn(const T& item, const std::set<T>& set)
{
    return set.find(item) == set.end();
}

// returns true when item is in set
template<typename T> 
bool itemIn(const T& item, const std::set<T>& set)
{
    return set.find(item) != set.end();
}


// a = a intersect b
template<typename T>
void and(std::set<T>& a, const std::set<T>& b)
{
    remove_if(a.begin(), a.end(), boost::bind(&itemNotIn<T>, _1, b));
}

// a = a subtract b
template<typename T>
void and_not(std::set<T>& a, const std::set<T>& b)
{
    remove_if(a.begin(), a.end(), boost::bind(&itemIn<T>, _1, b));
}

// a = a union b (with intersection boost factor)
template<typename T>
void or(std::set<T>& a, const std::set<T>& b, float intersection_boost)
{
    for (std::set<T>::const_iterator pb = b.begin(); pb != b.end(); pb++) {
        std::set<T>::iterator pa( a.find(*pb) );
        if (pa == a.end()) {
            a.insert(*pb);
        } else {
            pa->weight = intersection_boost * (pb->weight + pa->weight);
        }
    }
}

}

#endif