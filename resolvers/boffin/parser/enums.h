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
#ifndef __QUERY_PARSER_ENUMS_H
#define __QUERY_PARSER_ENUMS_H

namespace fm { namespace last { namespace query_parser {

/**
 * \brief Types of radio services supported by the query parser
 *
 * \attention This enum is mimicking _radio::TRadioService to make the
 * query parser less dependent on the radio itself. Don't update this enum
 * without altering the other copy in \c radio.thrift.
 */
enum RadioService
{
   RS_UNKNOWN          = 0,  ///< Unknown radio service

   RS_LIBRARY          = 1,  ///< Personal radio or library radio, port: 1501

   RS_LOVED            = 2,  ///< Radio of loved tracks, port: 1502
   RS_NEIGHBORS        = 3,  ///< Neighbour radio, port: 1503

   RS_PLAYLIST         = 4,  ///< Radio playing a given playlist. NOT supported here YET! Port: 1504

   RS_RECOMMENDED      = 5,  ///< Recommendation radio, port: 1505
   RS_SIMILAR_ARTISTS  = 6,  ///< Similar artists radio for a given artist, port: 1506

   RS_GLOBAL_TAG       = 7,  ///< Global tag radio for a given tag, port: 1507
   RS_USER_TAG         = 8,  ///< Tag radio of a user, port: 1508

   RS_GROUP            = 9,  ///< Radio of songs scrobbled by a given group, port: 1509
   RS_EVENT            = 10, ///< Radio of artists performing at a given event. NOT supported here YET! Port: 1510

   RS_HYPE             = 11, ///< Radio of hyped tracks, port 1511

   RS_ARTIST           = 56, ///< Special case: will return tracks from the given artist. It's used in the aggregator only!

   RS_AGGREGATOR       = 100 ///< Aggregator radio based on queries. Port: 1600
};

/**
 * \brief Operator types for aggregator queries
 *
 * Members of this enum represent the binary operators that can be found in an
 * aggregator query
 *
 * \attention This enum is mimicking what's in _radio::TAggregatorOpType
 * to make the query parser less dependent on the radio itself. Don't update
 * this enum without altering the other copy in \c radio.thrift.
 */
enum OpType
{
   OT_AND 	  = 1001,       ///< \c AND operator
   OT_OR  	  = 1002,       ///< \c OR operator
   OT_AND_NOT = 1003        ///< <tt>AND NOT</tt> operator
};

}}} // end of namespaces

#endif // __QUERY_PARSER_ENUMS_H
