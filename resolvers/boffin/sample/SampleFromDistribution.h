/***************************************************************************
 *   Copyright 2005-2009 Last.fm Ltd.                                      *
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

// this file courtesy norman and mir

// Sample from EMPIRICAL (given) distribution.
// There are three types of sampling here:
// linear: uses a simple linear search that does not guarantee
//         that the number of requested samples will be found.
//         Use it for LARGE distributions
// tree: Simple tree balanced on probability. The best solution for
//       small/medium distributions
// justrandom: simply pull random samples

#ifndef __SAMPLE_FROM_DISTRIBUTION_H
#define __SAMPLE_FROM_DISTRIBUTION_H

#include <algorithm>
#include <vector>
#include <ctime>
#include <limits>

#include <boost/random.hpp>

namespace fm { namespace last { namespace algo {

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

template < typename AccessPolicy, typename CopyPolicy >
class ListBasedSampler
{
   const CopyPolicy   m_copyElement;
   const AccessPolicy m_accessElement;

   boost::mt19937                            m_randomGenerator;
   // be very careful with this!! even if the methods are const
   // they are NOT thread safe due to this variable!
   mutable boost::uniform_01<boost::mt19937> m_uniform01Distr;

public:

   ListBasedSampler() :
      m_copyElement(),
      m_accessElement(),
      m_randomGenerator(),
      m_uniform01Distr(m_randomGenerator)
      {
         boost::uint32_t localSeed = time(0);
            //static_cast<boost::uint32_t>( reinterpret_cast<boost::int64_t>(this) %
            //(std::numeric_limits<boost::uint32_t>::max)() );

         m_uniform01Distr.base().seed(localSeed);
      }

   // -----------------------------------------------------------------------------

   // sample just one element from the distribution
   // isPDF assumes that the passed data is a probability distribution
   // function, and therefore the sum of all it's elements is = 1
   template <typename IT>
   typename std::iterator_traits<IT>::value_type
      singleSample( IT first, IT last,
                    bool isPDF = false ) const
   {
      double sum;
      if ( isPDF )
         sum = 1;
      else
      {
         sum = 0;
         for ( IT it = first; it != last; ++it )
            sum += m_accessElement(it);
      }

      double randPos = m_uniform01Distr() * sum;
      double summedPos = sum;

      IT foundIt;
      for ( foundIt = first; foundIt != last ; ++foundIt )
      {
         summedPos -= m_accessElement(foundIt);
         if ( randPos > summedPos )
            break;
      }

      if ( foundIt == last )
         return m_copyElement(first);
      else
         return m_copyElement(foundIt);
   }

};


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

}}} // end of namespaces

#endif // __SAMPLE_FROM_DISTRIBUTION_H

