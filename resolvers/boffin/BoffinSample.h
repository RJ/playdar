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

#include "ResultSet.h"
#include <boost/function.hpp>

#include "sample/SampleFromDistribution.h"


using namespace fm::last::algo;


struct AccessPolicy
{
  template <typename IT>
  float operator()(const IT& el) const
  { return el->second; }
};


struct CopyPolicy
{
   //template <typename InIT, typename OutIT>
   //void operator() ( const InIT& from, OutIT& to ) const
   //{
   //   *to = *from;
   //}

   template <typename IT>
   typename std::iterator_traits<IT>::value_type operator() ( IT& it ) const
   {
      return *it;
   }

   template <typename IT>
   const typename std::iterator_traits<const IT>::value_type operator() ( const IT& it ) const
   {
      return *it;
   }
};


// a trackresult and the 'working-weight'
typedef std::pair<const TrackResult*, float> Entry;       

bool orderByWeightDesc(const Entry& a, const Entry& b)
{
    return a.second > b.second;
}


// Returns a number of weighted samples from the ResultSet.
// The weight of each entry in the ResultSet is first multiplied
// by the pushdownFactor(const TrackResult&) function
//
// PushdownFactorFunctor is: float(int artist, int track)
// ResultFunctor is: void(TrackResult track)
//
template<typename PushdownFactorFunctor, typename ResultFunctor>
void
boffinSample(int count, const ResultSet& rs, PushdownFactorFunctor pushdown, ResultFunctor result)
{
    // convert the ResultSet to a vector for sorting
    int trackCount = rs.size();
    std::vector<Entry> tracks;
    tracks.reserve(trackCount);
    for (ResultSet::const_iterator it = rs.begin(); it != rs.end(); it++) {
        tracks.push_back(std::make_pair(&(*it), it->weight));
    }

    // pull out a single sample
    ListBasedSampler<AccessPolicy, CopyPolicy> sampler;

    for (; count && trackCount; count--, trackCount--) {
        std::vector<Entry>::iterator pBegin = tracks.begin();
        std::vector<Entry>::iterator pEnd = tracks.begin() + trackCount;
        std::vector<Entry>::iterator it;

        // reweight:
        float totalWeight = 0;
        for (it = pBegin; it != pEnd; it++) {
            it->second = it->first->weight * pushdown( *it->first );
            totalWeight += it->second;
        }
        // normalise weights, sum to 1
        for (it = pBegin; it != pEnd; it++) {
            it->second /= totalWeight;
        }
        std::sort(pBegin, pEnd, orderByWeightDesc);

        std::vector<Entry>::iterator pSample = sampler.singleSample(pBegin, pEnd, true);
        result( *(pSample->first) );

        // swap it with the bottom entry (so it gets ignored next time through)
        Entry temp = *pSample;
        *pSample = *(pEnd-1);
        *(pEnd-1) = temp;
    }
}

