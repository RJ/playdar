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

#ifndef __COS_SIMILARITY_H
#define __COS_SIMILARITY_H

#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>

// -----------------------------------------------------------------------------

namespace moost { namespace algo {

class CosSimilarity
{

public:

   // Compare 'base' to 'candidates'; callback on 'resultFun' to deliver 
   // the similarity score.  Uses accessPolicy to access a TEntry.
   // TEntry is effectively opaque to findSimilar.
   //
   // TEntryAccessPolicy has to implement
   //
   // TEntryIterator get_begin(const TEntry&)
   // TEntryIterator get_end(const TEntry&)
   // (IMPORTANT: the iterator range returned MUST be sorted by id!)
   //
   // int get_id(const TEntryIterator&)
   // float get_score(const TEntryIterator&)
   //
   // float get_norm(const TEntry&)
   // Note: here the pair contains the value from the similarity computation and the number of co-occurences
   // double apply_post_process( const TEntry& entryA, const TEntry& entryB, const std::pair<double, int>& simVal ); 
   //
   // note: the EntryAccessPolicy can contain other stuff as it is always copied by ref and
   //       treated as const
   //
   // TCandContainer must support begin() and end() methods
   //
   // 
   template< typename TResultFun, typename TEntry, typename TCandContainer, typename TEntryAccessPolicy>
   static void findSimilar( 
      TResultFun resultFun,
      const TEntry& base,
      const TCandContainer& candidates,
      const TEntryAccessPolicy& accessPolicy )
   {
      typename TCandContainer::const_iterator cIt;
      for ( cIt = candidates.begin(); cIt != candidates.end(); ++cIt )
      {
         resultFun( *cIt, (float) getCosOfAngle(base, *cIt, accessPolicy) );
      }
   }

   //////////////////////////////////////////////////////////////////////////
   //template< typename TEntry, typename TEntryAccessPolicy>
   //static double getNorm( const TEntry& entry, const TEntryAccessPolicy& accessPolicy )
   //{
   //   return getNorm( accessPolicy.get_begin(entry), accessPolicy.get_end(entry), accessPolicy );
   //}

   //////////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////////
private:

   template <typename TEntry>
   static bool comparer( const std::pair<TEntry, float>& p1, const std::pair<TEntry, float>& p2 )
   { return p1.second > p2.second; }

   //////////////////////////////////////////////////////////////////////////

   //template< typename TEntryIterator, typename TEntryAccessPolicy>
   //static double getNorm( TEntryIterator beg, TEntryIterator end, 
   //                       const TEntryAccessPolicy& accessPolicy )
   //{
   //   double norm = 0;
   //   for (; beg != end; ++beg)
   //      norm += accessPolicy.get_score(beg) * accessPolicy.get_score(beg);
   //   return std::sqrt(norm);
   //}

   //////////////////////////////////////////////////////////////////////////

   template< typename TEntry, typename TEntryAccessPolicy>
   static double getCosOfAngle( const TEntry& entryA, const TEntry& entryB,
                                const TEntryAccessPolicy& accessPolicy )
   {
      std::pair<double, int> simVal = getDotProduct(entryA, entryB, accessPolicy);
      simVal.first /= ( accessPolicy.get_norm(entryA) * accessPolicy.get_norm(entryB) );
      return accessPolicy.apply_post_process(entryA, entryB, simVal);
   }

   //////////////////////////////////////////////////////////////////////////

   template< typename TEntry, typename TEntryAccessPolicy>
   static std::pair<double, int>
      getDotProduct( const TEntry& entryA, const TEntry& entryB,
                     const TEntryAccessPolicy& accessPolicy )
   {
      return getDotProduct( accessPolicy.get_begin(entryA), accessPolicy.get_end(entryA), 
                            accessPolicy.get_begin(entryB), accessPolicy.get_end(entryB),
                            accessPolicy );
   }
   
   //////////////////////////////////////////////////////////////////////////

   template< typename TEntryIterator, typename TEntryAccessPolicy>
   static std::pair<double, int>
      getDotProduct( TEntryIterator begA, TEntryIterator endA,
                     TEntryIterator begB, TEntryIterator endB,
                     const TEntryAccessPolicy& accessPolicy )
   {
      std::pair<double, int> res(0,0);
      for (; begA != endA && begB != endB; )
      {
         if ( accessPolicy.get_id(begA) < accessPolicy.get_id(begB) )
            ++begA;
         else if ( accessPolicy.get_id(begB) < accessPolicy.get_id(begA) )
            ++begB;
         else
         {
            res.first += accessPolicy.get_score(begA) * accessPolicy.get_score(begB);
            ++(res.second);
            ++begA;
            ++begB;
         }
      }
      
      return res;
   }
};

}}

// -----------------------------------------------------------------------------

#endif // __COS_SIMILARITY_H

// -----------------------------------------------------------------------------
