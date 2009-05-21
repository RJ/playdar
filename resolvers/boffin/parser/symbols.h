/*
    Playdar - music content resolver
    Copyright (C) 2009  Last.fm Ltd.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __QUERY_PARSER_SYMBOLS_H
#define __QUERY_PARSER_SYMBOLS_H

#include "enums.h"
#include <boost/spirit/symbols/symbols.hpp>
using namespace boost::spirit;

namespace fm { namespace last { namespace query_parser {

// -----------------------------------------------------------------------------

/// Symbol table for service types, to be used in the parser
/**
 * This symbol table associates strings to corresponding TRadioService types.
 * Currently we understand the following service type strings:
 *
 * - \c group = RS_GROUP
 * - \c library = RS_LIBRARY
 * - \c loved = RS_LOVED
 * - \c love = RS_LOVED
 * - \c rec = RS_RECOMMENDED
 * - \c recs = RS_RECOMMENDED
 * - \c simart = RS_SIMILAR_ARTISTS
 * - \c tag = RS_GLOBAL_TAG
 * - \c user = RS_LIBRARY
 */
struct servicetype_syn : symbols<int>
{
   /// Constructs the symbol table
   servicetype_syn()
   {
      add
         ("library", RS_LIBRARY )
         ("user",    RS_LIBRARY )

         ("loved",   RS_LOVED )
         ("love",    RS_LOVED )

         ("neigh",      RS_NEIGHBORS )
         ("neighbors",  RS_NEIGHBORS )

         ("rec",     RS_RECOMMENDED )
         ("recs",    RS_RECOMMENDED )

         ("simart",  RS_SIMILAR_ARTISTS ) // similart artists
         ("tag",     RS_GLOBAL_TAG ) // global tag
         ("group",   RS_GROUP ) // groups radio

         ("art",     RS_ARTIST ) // just the tracks of the given artist

         //("event",   RS_EVENT )
         ;
   }
};

// -----------------------------------------------------------------------------

/// Symbol table for term operators, to be used in the parser
/**
 * This symbol table associates strings to corresponding TAggregatorOpType types.
 * Term operators have priority over expression operators.
 * Currently we understand the following term operators:
 *
 * - \c AND = \c AMT_AND
 * - \c NOT = \c AMT_AND_NOT
 * - \c AND \c NOT = \c AMT_AND_NOT
 */
struct term_ops_syn : symbols<int>
{
   /// Constructs the symbol table
   term_ops_syn()
   {
      add
         ("and",     OT_AND )
         ("not",     OT_AND_NOT )
         ("and_not", OT_AND_NOT )
         ;
   }
};

// -----------------------------------------------------------------------------

/// Symbol table for expression operators, to be used in the parser
/**
 * This symbol table associates strings to corresponding TAggregatorOpType types.
 * Term operators have priority over expression operators.
 * Currently we understand the following expression operators:
 *
 * - \c OR = \c AMT_OR
 */
struct expression_ops_syn : symbols<int>
{
   /// Constructs the symbol table
   expression_ops_syn()
   {
      add
         ("or", OT_OR )
         ;
   }
};

// -----------------------------------------------------------------------------

}}} // end of namespaces

// -----------------------------------------------------------------------------

#endif // __QUERY_PARSER_SYMBOLS_H
