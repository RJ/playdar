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
#ifndef __QUERY_PARSER_NODE_H
#define __QUERY_PARSER_NODE_H

#include <boost/spirit/tree/common_fwd.hpp>
#include <string>
#include "enums.h"

///////////////////////////////////////////////////////////////////////////////
//using namespace boost::spirit;

namespace fm { namespace last { namespace query_parser {

//////////////////////////////////

/// Own factory class for the parser to create \c boost::spirit::node_iter_data objects
template <typename ValueT>
class querynode_iter_data_factory
{
public:
   /** \brief Inner factory class so that \c boost::spirit::node_iter_data_factory
    *  can simulate a template template parameter */
   template <typename IteratorT>
   class factory
   {
   public:
      /// Local alias to IteratorT
      typedef IteratorT iterator_t;
      /// Shorthand notation to the type of nodes this factory makes
      typedef boost::spirit::node_iter_data<iterator_t, ValueT> node_t;

      /// Creates a node
      /**
       * \param  first  iterator pointing to the start of the string matched
       *                by the node we are constructing
       * \param  last   iterator pointing to the end of the string matched
       *                by the node we are constructing
       */
      static node_t create_node( iterator_t const& first, iterator_t const& last,
                                 bool /*is_leaf_node*/)
      {
         return node_t(first, last);
      }

      /// Returns an empty node
      static node_t empty_node()
      {
         return node_t();
      }

      /// Groups the given nodes into a single one and returns it
      /**
       * This is used by the \c leaf_node and \c token_node Spirit directives.
       * \tparam  ContainerT  template parameter for the container we use to
       *                      store the nodes to be grouped
       * \param   nodes       the list of nodes to be grouped
       * \pre  \a ContainerT contains only a single <tt>tree_node\<node_t\></tt>
       *       and all iterators in the container point to the same sequence.
       */
      template <typename ContainerT>
      static node_t group_nodes(ContainerT const& nodes)
      {
         return nodes.front().value;
      }
   };
};


// -----------------------------------------------------------------------------

/// Structure that stores all the information related to a given tree node while parsing \c radioql queries
/**
 * This structure relates to TAggregatorEntry: in the end, we will create
 * TAggregatorEntry instances from node_data structures
 *
 * \sa TAggregatorEntry
 */
struct querynode_data
{
   /// Constant used to tag token-related rules in the grammar
   static const int tokenID = 1;
   /// Constant used to tag operation-related rules in the grammar
   static const int operationID = 2;

   /// Construct a new querynode_data with unknown type and ID (-1) and weight 1.0
   querynode_data()
      : type(RS_UNKNOWN), weight(1.0), ID(-1)
   {}

   /// Clears the node
   /**
    * - Sets the type and the ID to unknown
    * - Sets the weight to 1.0
    * - Clears the name to an empty string
    */
   void clear()
   {
      type = RS_UNKNOWN;
      weight = 1.0;
      ID = -1;
      name.clear();
   }

   /// The type of the node (TAggregatorOpType or TRadioService enum values)
   mutable int         type;
   /// The weight corresponding to the node
   mutable double      weight;
   /// Seed ID when the parsed term specifies a seed ID instead of a name
   mutable int         ID;
   /// Seed by name when the parsed term specifies a name instead of a seed ID
   mutable std::string name;
};

// -----------------------------------------------------------------------------

/// The iterators we use in the parser are just \c char* pointers
typedef char const*                                      iterator_t;
/// A factory type similar to boost::spirit::node_iter_data_factory
/** This factory constructs <tt>boost::spirit::node_iter_data\<iterator_t, querynode_data\></tt>
 * objects; the part of the string we are parsing that matches a given tree
 * node is given by iterators, while the data we parsed out of the string
 * is stored in an internal <tt>querynode_data</tt> struct.
 */
typedef querynode_iter_data_factory<querynode_data>      factory_t;
/// Node types constructed by a factory of type \c factory_t
typedef boost::spirit::node_iter_data<iterator_t, querynode_data> node_t;

}}}

// -----------------------------------------------------------------------------

#endif // __QUERY_PARSER_NODE_H
