#ifndef __QUERY_PARSER_GRAMMAR_H
#define __QUERY_PARSER_GRAMMAR_H

#include <boost/spirit/core.hpp>
#include <boost/spirit/utility.hpp>

#include <boost/spirit/tree/parse_tree.hpp>
#include <boost/spirit/tree/ast.hpp>

#include <boost/version.hpp>

#include "symbols.h"
#include "node.h"

#include <string>

///////////////////////////////////////////////////////////////////////////////
using namespace boost::spirit;

namespace fm { namespace last { namespace query_parser {

// -----------------------------------------------------------------------------

/// Simple function-like structure to set a querynode_data in the parser
struct set_node
{
   /// Constructor, stores the data that will be assigned upon "calling" the struct
   set_node(querynode_data& toAssignData)
      : m_toAssignData(toAssignData) {}

   /// Stores the data passed in to the constructor in the given node
   /**
    * \param  node   node that will contain the data that was passed in
    * \param  begin  ignored, points to the start of the string that matched
    * \param  end    ignored, points to the end of the string that matched
    */
   void operator()( tree_node<node_t>& node,
                    iterator_t begin,
                    iterator_t end) const
   {
      node.value.value(m_toAssignData);
      m_toAssignData.clear();
   }

private:

   /// Temporary storage for a reference to the data we are assigning
   querynode_data& m_toAssignData;
};

// -----------------------------------------------------------------------------

/// The \c radioql grammar specification in boost::spirit
/**
 * The \c grammar\<\>  template in Spirit encapsulates a set of grammar
 * rules. Here we extend the \c grammar\<\> template with the rules of the
 * \c radioql language. Rules are set up in the constructor. We also have a
 * temporary querynode_data structure to store the data of the current node we are
 * parsing.
 *
 * \sa node_data for the structure which holds information about a tree node
 *               while parsing
 * \sa http://spirit.sourceforge.net/distrib/spirit_1_8_5/libs/spirit/doc/grammar.html
 *     for details about the \c grammar\<\> template. Note that you might have
 *     to adjust the version number above if there's a new Spirit release.
 */
struct query_grammar :
   public grammar<query_grammar>
{

   /// A nested template class that is expected by Spirit in a grammar specification
   /** This template contains the grammar rules. */
   template <typename ScannerT>
   struct definition
   {
      /// The constructor of the grammar definition that defines the grammar rules
      definition(query_grammar const& self)
      {

         // Grammar rule for matching a string literal
         string_literal =
               // string        ::= '"' , ( any character - '"' )+ , '"' ;
               // any character ::= ? all visible characters ? ;
               // lexeme_d switches the parser to character level (won't skip
               // whitespace). The parsed string is stored in self.tmpNode.name.
               // Empty strings are disallowed.
               lexeme_d
               [
                  ch_p('"') >>
                  (+( anychar_p - ch_p('"') ))
                  [ assign_a(self.tmpNode.name) ] >>
                  ch_p('"')
               ]
          ;

          // Grammar rule for matching an identifier (any token except the operations)
          identifier =
             // identifier ::= ( non-space printable character - operator )+ ;
             // operator   ::= ":" | '"' | "^" | "(" | ")" | "[" | "]" ;
             // An identifier is at lease one character long and it is stored
             // in self.tmpNode.name when parsing.
             lexeme_d
             [
                //*(alnum_p)
               +(graph_p -
                  // all the ops
                  ( ch_p(':')
                  | ch_p('"')
                  | ch_p('^')
                  | ch_p('(')
                  | ch_p(')')
                  | ch_p('[')
                  | ch_p(']')
                  )
               )
             ]
             [ assign_a(self.tmpNode.name) ]
          ;

          // Grammar rule for matching an integer in brackets
          number =
             // number ::= '[' , integer, ']' ;
             // The number itself is assigned to self.tmpNode.ID when parsing
             lexeme_d
             [
                ch_p('[') >>
                  (int_p)[assign_a(self.tmpNode.ID)] >>
                ch_p(']')
             ]
         ;

         // Grammar rule matching a query token. Examples are:
         // tag:rock^3.4
         // library:[24345]^1.2
         token =
            // token       ::= [ servicetype, ":" ] , ( string | identifier | number ) , [ "^", real ] ;
            // servicetype ::= "simart" | "tag" | "user" | "library" | "group"
            //
            // The token will always be a leaf node in the resulting abstract
            // syntax tree (AST). It consists of a mandatory part (which is
            // either a string literal, an identifier or a number in brackets),
            // an optional service type part (preceding the mandatory part and
            // separated by colons) and an optional weight (following the
            // mandatory part, separated by a caret.
            // servicetype_syn_p is a separate custom parser for service types;
            // it is practically a list of valid symbols accepted for service
            // types. They are considered case insensitive.
            // The parsed leaf node is stored in self.tmpNode; this is made
            // possible by \c access_node_d (allows us to attach an action to a
            // tree node) and \c set_node that we defined.

#if BOOST_VERSION >= 103500
            reduced_node_d
#else
            leaf_node_d
#endif
            [
               access_node_d
               [
                  !( as_lower_d[servicetype_syn_p]
                     [assign_a(self.tmpNode.type)]
                      >> ch_p(':')
                   ) >>
                  ( string_literal | identifier | number ) >>
                  !( ch_p('^') >>
                     ureal_p
                     [assign_a(self.tmpNode.weight)]
                   )
               ][set_node(self.tmpNode)]
            ]
         ;

         // //////////////////////////////////////////////////////////////////////////

         ////grouped =
         ////   discard_node_d
         ////   [
         ////       !( as_lower_d[servicetype_syn_p]
         ////          [assign_a(self.tmpNode.type)]
         ////           >> ch_p(':')
         ////        )
         ////   ]
         ////   >> inner_node_d[ch_p('(') >> expression >> ch_p(')') ] >>
         ////   discard_node_d
         ////   [
         ////    !( ch_p('^') >>
         ////       ureal_p
         ////       [assign_a(self.tmpNode.weight)]
         ////     )
         ////   ]
         //// ;

         // Grammar rule matching a factor (either an expression in parentheses
         // or a single token, see above).
         // factor ::= '(' , expression, ')' | token
         // The \c inner_node_d directive in the source causes the first and
         // the last child node in the abstract syntax tree to be ignored,
         // since we don't really care about the parentheses themselves.
         factor
             = inner_node_d[ch_p('(') >> expression >> ch_p(')')]
              //grouped
              |  token
         ;

         // Grammar rule matching a term (a list consisting of factors
         // concatenated by term operators). Operators are parsed by
         // termop_syn_p and they become root nodes in the syntax tree.
         // Term operators are case insensitive.
         // term   ::= factor , ( termop , factor )*
         // termop ::= "and" | "not" | "and not"
         term
             =   factor
                 >> *(
                       access_node_d
                       [
                           root_node_d
                           [
                              as_lower_d[termop_syn_p]
                              [assign_a(self.tmpNode.type)]
                           ]
                       ][set_node(self.tmpNode)]
                       >> factor
                     )
         ;

         // Grammar rule matching an expression (a list consisting of terms
         // concatenated by expression operators). Operators are parsed by
         // expressionop_syn_p and they become root nodes in the syntax tree.
         // Expression operators are case insensitive.
         // expression   ::= term , ( expressionop , term )*
         // expressionop ::= "or"
         expression
            =   term
                >> *(
                     ( root_node_d
                       [
                          access_node_d
                          [
                             as_lower_d[expressionop_syn_p]
                             [assign_a(self.tmpNode.type)]
                          ][set_node(self.tmpNode)]
                       ]
                       >> term )
                    )
         ;

         //  End grammar definition

         //// turn on the debugging info.
         //BOOST_SPIRIT_DEBUG_RULE(token);
         //BOOST_SPIRIT_DEBUG_RULE(factor);
         //BOOST_SPIRIT_DEBUG_RULE(term);
         BOOST_SPIRIT_DEBUG_RULE(expression);
      }

      /// Grammar rule matching a string literal, initialised in the constructor
      rule<ScannerT, parser_tag<querynode_data::tokenID> > string_literal;
      /// Grammar rule matching an identifier, initialised in the constructor
      /** An identifier is a non-empty sequence of printable non-whitespace
       * characters excluding operators (caret, parentheses, colon).
       */
      rule<ScannerT, parser_tag<querynode_data::tokenID> > identifier;
      /// Grammar rule matching an integer number in brackets, initialised in the constructor
      rule<ScannerT, parser_tag<querynode_data::tokenID> > number;
      /// Grammar rule matching a token, initialised in the constructor
      /** A token looks like <tt>[servicetype:]identifier[^weight]</tt>, where
       * <tt>servicetype</tt> is a valid service type in the
       * radioql_grammar::servicetype_syn_p symbol table and weight is a positive
       * real number. See the more precise EBNF form in the body of the
       * constructor.
       */
      rule<ScannerT, parser_tag<querynode_data::tokenID> > token;

      /// Grammar rule matching an expression
      /**
       * In \c radioql, we use the traditional expression \> term \> factor
       * structure that is frequently used for arithmetic grammars taught in
       * CS textbooks. Expressions consist of terms separated by expression
       * operators, terms consist of factors separated by term operators.
       * Term operators take precedence over expression operators.
       * In our case, the only expression operator is \c OR, while \c AND,
       * \c NOT and <tt>AND NOT</tt> are term operators.
       */
      rule<ScannerT, parser_tag<querynode_data::operationID> > expression;
      /// Grammar rule matching a term
      /** \sa expression */
      rule<ScannerT, parser_tag<querynode_data::operationID> > term;
      /// Grammar rule matching a factor
      /** \sa expression */
      rule<ScannerT, parser_tag<querynode_data::operationID> > factor;

      /// Symbol table for valid term operators
      term_ops_syn       termop_syn_p;
      /// Symbol table for valid expression operators
      expression_ops_syn expressionop_syn_p;
      /// Symbol table for valid service types
      servicetype_syn    servicetype_syn_p;

      /// Returns the starting rule for this grammar
      rule<ScannerT, parser_tag<querynode_data::operationID> > const&
         start() const { return expression; }
   };

private:
   /// The current node we are parsing
   mutable querynode_data tmpNode;

};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

}}} // end of namespaces

// -----------------------------------------------------------------------------

#endif //__QUERY_PARSER_GRAMMAR_H

