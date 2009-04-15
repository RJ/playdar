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

#include "Plist.h"
#include <string>
#include <sstream>

static const std::string base64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void Plist::read( std::istream &in)
{
    //std::string buffer;
    std::stringbuf buffer;
    char tmp;
    while( in.good() )
    {
        in >> tmp;
        if(  tmp == '<' && ( in.peek() == '?' || in.peek() == '!' ) )
        {
            in.get( buffer, '>' );
            in.get();
            continue;
        }
        else
            in.putback( tmp );

        m_root = Element( in );
    }
}

void Plist::write( std::ostream& out ) const
{
    out << m_root;
}

Element::~Element()
{
    if( m_type == DATA )
        delete[] m_dataPtr;
}


Element& 
Element::operator=( const Element& element )
{
    m_type = element.m_type;
    m_string = element.m_string;
    m_dataLength = element.m_dataLength;
    m_dict = element.m_dict;
    m_string = element.m_string;
    m_array = element.m_array;
    m_indent = element.m_indent;

    if( element.m_type == DATA )
    {
        m_dataPtr = new char[ m_dataLength ];
        memcpy( m_dataPtr, element.m_dataPtr, m_dataLength );
    }
    return *this;
}


std::string //static
Element::trim( const std::string& str )
{
    std::string retVal;
    std::string::const_iterator iter;
    for( iter = str.begin(); iter != str.end(); iter++ )
    {
        if( *iter != ' ' && 
            *iter != '\n' &&
            *iter != '\t' )
            retVal+= *iter;
    }
    return retVal;
}


void 
Element::read(std::istream &in)
{
    std::string elementName = readElementName( in );

    if( elementName.find( "<plist" ) == 0 )
    {
        m_type = PLIST;
        while( in.good() )
        {
            m_array.push_back( Element( in ) );
        }
    }
    else if( elementName.find( "<string" ) == 0 )
    {
        m_type = Element::STRING;
        m_string = readElementContents( in );
    }
    else if( elementName.find( "<data" ) == 0 )
    {
        m_type = Element::DATA;
        m_string = trim( readElementContents( in ) );
        m_dataLength = (unsigned int(m_string.length()) / 4) * 3;
        m_dataPtr = new char[ m_dataLength ];
        base64decode( m_string, m_dataPtr );
    }
    else if( elementName.find( "<date" ) == 0 )
    {
        m_type = Element::DATE;
        m_string = readElementContents( in );
    }
    else if( elementName.find( "<dict" ) == 0 )
    {
        m_type = Element::DICT;
        std::string curElement = readElementName( in );
        while( curElement.find( "</dict>" ) == std::string::npos )
        {
            if( curElement.find( "<key" ) == std::string::npos )
                throw std::string( "Unexpected element - expecting key" );

            std::string key = readElementContents( in );
            
            curElement = readElementName( in );

            if( curElement.find( "</key>" ) == std::string::npos )
                throw std::string( "Parsing error: Missed match key tags." );

            m_dict[ key ] = Element( in );
            m_dict[ key ].setIndent( m_indent + 1 );
            curElement = readElementName( in );
        }
        return;
    }
    //remove closing tag from the stream
    //TODO: check for mismatched elements?
    readElementName( in );
}


std::string 
Element::readElementName( std::istream& in ) const
{
    std::stringbuf buffer;
    in.get( buffer, '>' );
    std::string elementName = trim( buffer.str() );
    elementName += in.get();
    return elementName;
}


std::string 
Element::readElementContents( std::istream& in ) const
{
    std::stringbuf buffer;
    in.get( buffer, '<' );
    return buffer.str();
}


void
Element::write(std::ostream &out) const
{
    switch( m_type )
    {
    case PLIST:
    case ARRAY:
        {
            std::vector<Element>::const_iterator iter;
            for( iter = m_array.begin(); iter != m_array.end(); iter++ )
            {
                out << *iter << std::endl;
            }
        }
        break;
    case DICT:
        {
            std::map< std::string, Element >::const_iterator iter;
            for( iter = m_dict.begin(); iter != m_dict.end(); iter++ )
            {
                out << std::endl;
                for( int i = 0; i < m_indent; i++ )
                    out << '\t';

                out << iter->first << " = " << iter->second << std::endl;
            }
        }
        break;

    case STRING:
    case DATE:
    case DATA:
        out << m_string;
        break;
    }
}


void
Element::base64decode( std::string &in, char *out )
{
    if( in.length() > 0 && in.length() % 4 )
    {
        throw std::string( "Error cannot convert from base64 - wrong character length" );
    }
    std::string::iterator iter;
    char n[4];
    unsigned int outIndex = 0;

    for( iter = in.begin(); iter != in.end(); )
    {
        for( int i = 0; i < 4; i++ )
        {
            n[i] = *(iter++);

            // the '=' character is used as padding in base64
            if( n[i] == '=' )
            {
                n[i] = 0;
                continue;
            }

            //get the index of the character
            n[i] = base64chars.find( n[i] );
        }
        
        //redistribute the 4 x 6byte values into 3 x 8byte values
        //and insert into output buffer.
        out[outIndex++] = ( n[0] << 2) + ((n[1] & 0x30) >> 4);
        out[outIndex++] = ((n[1] & 0xf) << 4) + ((n[2] & 0x3c) >> 2);
        out[outIndex++] = ((n[2] & 0x3) << 6) + n[3];
    }
}


Element& 
Element::operator[]( const std::string& key )
{ 
    if( m_type != DICT ) 
        throw std::string( "Could not access key from non dictionary node." ); 
    
    return m_dict[ key ];
}


Element& 
Element::operator[]( int index )
{ 
    if( m_type != ARRAY && 
        m_type != PLIST )
    {
        throw std::string( "Can only access elements by index with Array or Plist elements." );
    }
    return m_array[ index ];
}


const char * const
Element::getData() const
{
    if( m_type != DATA )
    {
        throw std::string( "Can only access data from DATA elements" );
    }
    return m_dataPtr;
}
