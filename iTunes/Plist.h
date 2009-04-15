/***************************************************************************
 *   Copyright 2008-2009 Last.fm Ltd.                                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef PLIST_H
#define PLIST_H

#include <iostream>
#include <map>
#include <vector>


/** @author Jono Cole <jono@last.fm>
  * @brief Represents a single plist/array/dict/data/string/date element
  *        and can read / write from a stream
  */
class Element
{
public:
    Element():m_indent(0){}

    /** Construct an element by reading from an istream */
    explicit Element( std::istream& in ):m_indent(0){ read( in ); }

    ~Element();

    /** Retrieve an indexed element from an array or plist type element */
    Element& operator[]( int index );

    /** Retrieve an element from a dictionary based on a key */
    Element& operator[]( const std::string& key );

    /** set the indent size (used for pretty formatting when debugging!) */
    void setIndent( int indent ){ m_indent = indent; }

    /** get data length **/
    const int getDataLength() const{ return m_dataLength; }

    /** get a pointer the data stored in a data element */
    const char * const getData() const;

    /** Populate an Element from data in an istream */
    void read( std::istream& in );

    /** Write a prettified representation of the element to an ostream */
    void write( std::ostream& out ) const;

    /** Copy constructor calls operator= */
    Element( const Element& element ){ *this = element; }

    /** Overloaded operator= to make sure that data is copied if 
      * the element is a data element */
    Element& operator=( const Element& element );

private:
    enum{ STRING = 0,
      DATA,
      DATE,
      ARRAY,
      DICT,
      PLIST} m_type;

    std::string m_string;
    std::vector<Element> m_array;
    std::map< std::string, Element > m_dict;
    
    unsigned int m_dataLength;
    char* m_dataPtr;

    int m_indent;

    /** read the next element name from the stream
      * (ie <dict>, <plist>, <string> etc.)
	  * NOTE calls trim on the elementName
	  * TODO name with trim in it
      */
    std::string readElementName( std::istream& in ) const;

    /** read the conents of the element ie the data between 
      * <data> and </data> elements
      */
    std::string readElementContents( std::istream& in ) const;
    
	/** removes all whitespace, including internal whitespace */
    static std::string trim( const std::string& str );

    /** decode the base64 encoded in string into the out buffer */
    void base64decode( std::string& in, char* out );
};

inline std::istream& operator>>( std::istream& in, Element& element ){ element.read( in ); return in; }
inline std::ostream& operator<<( std::ostream& out, const Element& element ){ element.write( out ); return out; }


/** @author Jono Cole <jono@last.fm>
  * @brief Represents a single plist file and can read / write from a stream
  *        using m_root as the root node.
  */
class Plist
{
public:
    Plist(){}
    Plist( std::istream& in ){ read( in ); }
    ~Plist(void){};
    void read( std::istream& in );
    void write( std::ostream& out ) const;

    template <class T>
        Element& operator[]( T index ){ return m_root[ index ]; }

private:
    Element m_root;
};

inline std::istream& operator>>( std::istream& in, Plist& plist ){ plist.read( in ); return in; }
inline std::ostream& operator<<( std::ostream& out, const Plist& plist ){ plist.write( out ); return out; }

#endif // PLIST_H