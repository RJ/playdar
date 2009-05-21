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
#ifndef __QUERY_PARSER_PARSER_H
#define __QUERY_PARSER_PARSER_H

#include "node.h"
#include "enums.h"

#include <boost/spirit/tree/parse_tree.hpp>
#include <boost/spirit/error_handling.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <vector>
#include <string>
#include <map>

//#include <google/sparse_hash_map>
#include <boost/functional/hash.hpp>
#include <boost/function.hpp>

/**
 * \namespace fm::last::query_parser
 * \brief Query parser for the \c radioql language
 */
namespace fm { namespace last { namespace query_parser {

struct query_grammar;

/// Parser that parses \c radioql queries
/**
 * The parser is implemented using the tools provided in boost::spirit. If
 * you feel like you don't understand a single word from the comments in
 * the parser classes, read the documentation of boost::spirit first.
 *
 * The parser uses Spirit to turn the string to be parsed into an abstract
 * syntax tree representation using parser::parse(). The abstract syntax tree
 * representation cannot be queried directly; the data structure used by the
 * radio services is a vector of operation (typically TAggregatorEntry)
 * instances which represent the prefix traversal of the syntax tree. Since one
 * can unambiguously decide whether a given TAggregatorEntry is a leaf node
 * (its \c type field is one of the values defined in the TRadioService enums)
 * or not, the vector representation is completely equivalent to the AST.
 *
 * Roughly speaking, the source code of the parser can be divided into the
 * following subunits:
 *
 * - symbols.h: the symbol tables we use for the operators and the source names
 * - enums.h:  the enums we use for representing the operators and the radio
 *   sources as integers
 * - node.h:  the data structures we have for a single tree node in the abstract
 *   syntax tree
 * - grammar.h:  rules of the parser grammar in Spirit's syntax
 * - parser.h:  the parser interface itself
 *
 * The grammar accepted by the \c radioql parser in an EBNF-like (but maybe
 * not fully accurate) syntax is as follows:
 * \verbatim
expression   ::= term , { expressionop , term } ;
expressionop ::= "OR" ;
term         ::= factor , { termop , factor } ;
termop       ::= "AND" | "AND_NOT" ;
factor       ::= "(" , expression, ")" | token ;
token        ::= [ servicetype, ":" ] , ( identifier | string | number ) , [ "^" , weight ] ;
servicetype  ::= ? see symbols.h for a list of accepted service types ?
identifier   ::= identchar , { identchar }
identchar    ::= ? all printable non-whitespace characters ? - operator
operator     ::= "(" | ")" | "[" | "]" | ":" | "^"
string       ::= '"' , nonquote , { nonquote } , '"'
number       ::= "[" , int_p , "]"
weight       ::= ureal_p
nonquote     ::= ? all characters ? - "'"
\endverbatim
 *
 * where \c int_p is Spirit's built-in integer parser and \c ureal_p is Spirit's
 * built-in unsigned real number parser. \c expressionop, \c termop and
 * \c servicetype are case insensitive and are implemented by symbol tables.
 *
 * \sa http://spirit.sourceforge.net/documentation.html for the Spirit docs
 * \note The query parser is \em NOT thread safe!
 */
class parser
{
public:

   /// Constructs the parser
   parser();

   /// Sets the mapping function from tag names to IDs
   /** 
   * \param  func  the map function to use (must get a const std::string& and return int)
   * \note you can use boost::bind to set the function!
   * \important must return -1 if there's no valid mapping
   */
   template <typename TFunc>
   void setTagMappings(TFunc func)
   { m_tagMappingsFunc = func; }

   /// Sets the mapping function from artist names to IDs
   /** 
   * \param  func  the map function to use (must get a const std::string& and return int)
   * \note you can use boost::bind to set the function!
   * \important must return -1 if there's no valid mapping
   */
   template <typename TFunc>
   void setArtistMappings(TFunc func)
   { m_artistMappingsFunc = func; }

   /// Sets the mapping function from user names to IDs
   /** 
   * \param  func  the map function to use (must get a const std::string& and return int)
   * \note you can use boost::bind to set the function!
   * \important must return -1 if there's no valid mapping
   */
   template <typename TFunc>
   void setUserMappings(TFunc func)
   { m_userMappingsFunc = func; }

   /// Parses the given \c radioql query
   /** \param  str  the query to be parsed
    * \return  whether the query was fully parsed (\c true) or we had to stop
    *          prematurely due to an error (\c false)
    */
   bool        parse(const std::string& str);

   /// Returns the line number of the last error (if any)
   /** \return 0 if there was no error, otherwise it returns the line number
    *          where the error occurred
    */
   int         getErrorLineNumber();

   /// Returns the line itself where the last error occurred (if any)
   /** \return an empty string if there was no error, otherwise it returns the
    *          line where the parsers stopped.
    */
   std::string getErrorLine();

   /// Returns the character offset of the last error from the start of the string
   /** \return 0 if there was no error, otherwise it returns the character offset
    *          of the last error
    */
   int         getErrorOffset();

   /// Returns a vector representation of the currently parsed query
   /**
    * This function practically just calls parser::eval_expression with the
    * root of the tree.
    *
    * \tparam      T            the operator type we use in the vector representation
    * \param[out]  ops          the vector representation is returned here. The
    *                           vector will \em not be cleared
    * \param[in]   rootToOpFun  Boost function that tells us how to turn a root
    *                           node to an operator for the vector representation
    * \param[in]   leafToOpFun  Boost function that tells us how to turn a leaf
    *                           node to an operator for the vector representation
    * \param[in]   swapTagBranches make sure that in case a branch contains only tags
    *                              and the operation is an AND/ANDNOT, the tag is set
    *                              on the right side of the branch. This is useful when
    *                              the tag operation act as "filter" over what has been
    *                              fetched.
    */
   template <typename T>
   void getOperations(
      const boost::function< void( const T& t )>& accumulateFun,
      const boost::function< T( const querynode_data& node )>& rootToOpFun,
      const boost::function< T( const querynode_data& node )>& leafToOpFun,
      bool swapTagBranches = false );

   /// Returns the name of an operator, given the operator type
   /**
    * \param   opType  the type of the operator (an enum value from TAggregatorOpType)
    * \return  the operator name as a string
    */
   std::string getOpName(int opType);

   /// Returns the name of a service, given the service type
   /**
    * \param   serviceType  the type of the service (an enum value from TRadioService)
    * \return  the service name as a string
    */
   std::string getServiceName(int serviceType);

private:

   /// Shorthand notation for the resulting abstract syntax tree
   typedef boost::spirit::tree_match<iterator_t, factory_t>     parse_tree_match_t;
   /// Shorthand notation for an iterator over the resulting abstract syntax tree
   typedef parse_tree_match_t::tree_iterator                    tree_iter_t;

   /// Returns a vector representation of some part of the currently parsed query
   /**
    * \tparam      T            the operator type we use in the vector representation
    * \param[out]  ops          the vector representation is returned here
    * \param[in]   rootToOpFun  Boost function that tells us how to turn a root
    *                           node to an operator for the vector representation
    * \param[in]   leafToOpFun  Boost function that tells us how to turn a leaf
    *                           node to an operator for the vector representation
    * \param       it           iterator pointing to the node of the tree that is
    *                           the root of the expression we are processing
    * \param[in]   swapTagBranches make sure that in case a branch contains only tags
    *                              and the operation is an AND/ANDNOT, the tag is set
    *                              on the right side of the branch. This is useful when
    *                              the tag operation act as "filter" over what has been
    *                              fetched.
    */
   template <typename T>
   void eval_expression( const boost::function< void( const T& t )>& accumulateFun,
                         const boost::function< T( const querynode_data& node )>& rootToOpFun,
                         const boost::function< T( const querynode_data& node )>& leafToOpFun,
                         tree_iter_t const& it,
                         bool swapTagBranches );

   /// Returns the ID corresponding to the given key from the given map
   /**
    * \param  pItemMapper  the name-to-ID mapping
    * \param  key          the key we are looking for
    * \param  forceLowerCase will convert the key to lowercase before checking
    * \return  the ID or -1 if the key was not found
    */
   int  getID( boost::function< int(const std::string&) >& mapperFunc, 
               const std::string& key, bool forceLowerCase = true );

   /// returns true if the given branch contains only tag-based leaves
   inline bool isTagBranch( tree_iter_t const& it );

private:

   /// Function storing the mapping from tag names to IDs
   boost::function< int(const std::string&) > m_tagMappingsFunc;
   /// Function storing the mapping from artist names to IDs
   boost::function< int(const std::string&) > m_artistMappingsFunc;
   /// Function storing the mapping from user names to IDs
   boost::function< int(const std::string&) > m_userMappingsFunc;

   /// Iterator of the string we are parsing (or that we parsed the last time)
   iterator_t  m_pStringToParse;
   /// Last error message coming from the parser itself
   std::string errorMsg;

   /// Shared pointer to the \c radioql grammar we are using
   boost::shared_ptr<query_grammar> m_pGrammar;
   /// A boost::spirit::tree_parse_info holding the information related to the last parse attempt
   boost::spirit::tree_parse_info<iterator_t, factory_t> m_pi;
};

// -----------------------------------------------------------------------------

template <typename T>
void parser::getOperations(
   const boost::function< void( const T& t )>& accumulateFun,
   const boost::function< T( const querynode_data& node )>& rootToOpFun,
   const boost::function< T( const querynode_data& node )>& leafToOpFun,
   bool swapTagBranches /*= true */)
{
   eval_expression(accumulateFun, rootToOpFun, leafToOpFun, m_pi.trees.begin(), swapTagBranches);
}

// -----------------------------------------------------------------------------


template <typename T>
void parser::eval_expression(
   const boost::function< void( const T& t )>& accumulateFun,
   const boost::function< T( const querynode_data& node )>& rootToOpFun,
   const boost::function< T( const querynode_data& node )>& leafToOpFun,
   tree_iter_t const& it,
   bool swapTagBranches )
{
   T op;
   querynode_data currNode = it->value.value(); // make a copy

   if ( it->value.id() == querynode_data::operationID )
   {
      op = rootToOpFun(currNode);
      //if ( currNode.name.empty() )
      //   currNode.name = getOpName(currNode.type);

      accumulateFun(op);
      if ( it->children.size() != 2 )
         throw std::runtime_error("radioql_parser: Invalid number of children for operator!");

      if ( swapTagBranches && 
           currNode.type != OT_OR &&
           isTagBranch(it->children.begin()) )
      {
         // swap branches!
         eval_expression(accumulateFun, rootToOpFun, leafToOpFun, it->children.begin()+1, swapTagBranches);
         eval_expression(accumulateFun, rootToOpFun, leafToOpFun, it->children.begin()  , swapTagBranches);
      }
      else
      {
         eval_expression(accumulateFun, rootToOpFun, leafToOpFun, it->children.begin(),   swapTagBranches);
         eval_expression(accumulateFun, rootToOpFun, leafToOpFun, it->children.begin()+1, swapTagBranches);
      }

   }
   else if ( it->value.id() == querynode_data::tokenID )
   {

      if ( currNode.ID < 0 )
      { // no ID specified: let's try to set it from the mappings
         switch ( currNode.type )
         {
         case RS_LIBRARY:
         case RS_LOVED:
         case RS_NEIGHBORS:
         case RS_RECOMMENDED:
            currNode.ID = getID(m_userMappingsFunc, currNode.name);
            break;

         case RS_GLOBAL_TAG:
            currNode.ID = getID(m_tagMappingsFunc, currNode.name);
            break;

         case RS_SIMILAR_ARTISTS:
         case RS_ARTIST:
            currNode.ID = getID(m_artistMappingsFunc, currNode.name);
            break;
         }
      }

      op = leafToOpFun(currNode);
      accumulateFun(op);
   }
   else
      throw std::runtime_error("radioql_parser: Invalid node ID!");
}

// -----------------------------------------------------------------------------

bool parser::isTagBranch( tree_iter_t const& it )
{
   if ( it->value.id() == querynode_data::operationID )
   {
      return isTagBranch(it->children.begin()) && 
             isTagBranch(it->children.begin()+1);
   }
   else if ( it->value.id() == querynode_data::tokenID )
   {
      const querynode_data& currNode = it->value.value(); //ref!
      if ( currNode.type == RS_GLOBAL_TAG )
         return true;
      else
         return false;
   }
   else
      throw std::runtime_error("radioql_parser: Invalid node ID!");

   return false;
}

// -----------------------------------------------------------------------------

//void eval_expression( std::vector<TAggregatorEntry>& ops, tree_iter_t const& it );


}}} // namespaces

#endif // __QUERY_PARSER_PARSER_H

// -----------------------------------------------------------------------------
