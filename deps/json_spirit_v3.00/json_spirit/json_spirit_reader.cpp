/* Copyright (c) 2007-2009 John W Wilkinson

   This source code can be used for any purpose as long as
   this comment is retained. */

// json spirit version 3.00

#include "json_spirit_reader.h"
#include "json_spirit_value.h"

#define BOOST_SPIRIT_THREADSAFE  // uncomment for multithreaded use, requires linking to boost.thead

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/spirit/core.hpp>
#include <boost/spirit/utility/confix.hpp>
#include <boost/spirit/utility/escape_char.hpp>
#include <boost/spirit/iterator/multi_pass.hpp>
#include <boost/spirit/iterator/position_iterator.hpp>

using namespace json_spirit;
using namespace std;
using namespace boost;
using namespace boost::spirit;

//

Error_position::Error_position()
:   line_( 0 )
,   column_( 0 )
{
}

Error_position::Error_position( unsigned int line, unsigned int column, const std::string& reason )
:   line_( line )
,   column_( column )
,   reason_( reason )
{
}

bool Error_position::operator==( const Error_position& lhs ) const
{
    if( this == &lhs ) return true;

    return ( reason_ == lhs.reason_ ) &&
           ( line_   == lhs.line_ ) &&
           ( column_ == lhs.column_ ); 
}

//

namespace
{
    const int_parser< int64_t > int64_p = int_parser< int64_t >();

    template< class Iter_t >
    bool is_eq( Iter_t first, Iter_t last, const char* c_str )
    {
        for( Iter_t i = first; i != last; ++i, ++c_str )
        {
            if( *c_str == 0 ) return false;

            if( *i != *c_str ) return false;
        }

        return true;
    }

    template< class Char_t >
    Char_t hex_to_num( const Char_t c )
    {
        if( ( c >= '0' ) && ( c <= '9' ) ) return c - '0';
        if( ( c >= 'a' ) && ( c <= 'f' ) ) return c - 'a' + 10;
        if( ( c >= 'A' ) && ( c <= 'F' ) ) return c - 'A' + 10;
        return 0;
    }

    template< class Char_t, class Iter_t >
    Char_t hex_str_to_char( Iter_t& begin )
    {
        const Char_t c1( *( ++begin ) );
        const Char_t c2( *( ++begin ) );

        return ( hex_to_num( c1 ) << 4 ) + hex_to_num( c2 );
    }       

    template< class Char_t, class Iter_t >
    Char_t unicode_str_to_char( Iter_t& begin )
    {
        const Char_t c1( *( ++begin ) );
        const Char_t c2( *( ++begin ) );
        const Char_t c3( *( ++begin ) );
        const Char_t c4( *( ++begin ) );

        return ( hex_to_num( c1 ) << 12 ) + 
               ( hex_to_num( c2 ) <<  8 ) + 
               ( hex_to_num( c3 ) <<  4 ) + 
               hex_to_num( c4 );
    }

    template< class String_t >
    void append_esc_char_and_incr_iter( String_t& s, 
                                        typename String_t::const_iterator& begin, 
                                        typename String_t::const_iterator end )
    {
        typedef typename String_t::value_type Char_t;
             
        const Char_t c2( *begin );

        switch( c2 )
        {
            case 't':  s += '\t'; break;
            case 'b':  s += '\b'; break;
            case 'f':  s += '\f'; break;
            case 'n':  s += '\n'; break;
            case 'r':  s += '\r'; break;
            case '\\': s += '\\'; break;
            case '/':  s += '/';  break;
            case '"':  s += '"';  break;
            case 'x':  
            {
                if( end - begin >= 3 )  //  expecting "xHH..."
                {
                    s += hex_str_to_char< Char_t >( begin );  
                }
                break;
            }
            case 'u':  
            {
                if( end - begin >= 5 )  //  expecting "uHHHH..."
                {
                    s += unicode_str_to_char< Char_t >( begin );  
                }
                break;
            }
        }
    }

    template< class String_t >
    String_t substitute_esc_chars( typename String_t::const_iterator begin, 
                                   typename String_t::const_iterator end )
    {
        typedef typename String_t::const_iterator Iter_t;

        if( end - begin < 2 ) return String_t( begin, end );

        String_t result;
        
        result.reserve( end - begin );

        const Iter_t end_minus_1( end - 1 );

        Iter_t substr_start = begin;
        Iter_t i = begin;

        for( ; i < end_minus_1; ++i )
        {
            if( *i == '\\' )
            {
                result.append( substr_start, i );

                ++i;  // skip the '\'
             
                append_esc_char_and_incr_iter( result, i, end );

                substr_start = i + 1;
            }
        }

        result.append( substr_start, end );

        return result;
    }

    template< class String_t >
    String_t get_str_( typename String_t::const_iterator begin, 
                       typename String_t::const_iterator end )
    {
        assert( end - begin >= 2 );

        typedef typename String_t::const_iterator Iter_t;

        Iter_t str_without_quotes( ++begin );
        Iter_t end_without_quotes( --end );

        return substitute_esc_chars< String_t >( str_without_quotes, end_without_quotes );
    }

    string get_str( string::const_iterator begin, string::const_iterator end )
    {
        return get_str_< string >( begin, end );
    }

    wstring get_str( wstring::const_iterator begin, wstring::const_iterator end )
    {
        return get_str_< wstring >( begin, end );
    }
    
    template< class String_t, class Iter_t >
    String_t get_str( Iter_t begin, Iter_t end )
    {
        const String_t tmp( begin, end );  // convert multipass iterators to string iterators

        return get_str( tmp.begin(), tmp.end() );
    }

    // this class's methods get called by the spirit parse resulting
    // in the creation of a JSON object or array
    //
    // NB Iter_t could be a std::string iterator, wstring iterator, a position iterator or a multipass iterator
    //
    template< class String_t, class Iter_t >
    class Semantic_actions 
    {
    public:

        typedef Value_impl< String_t >        Value_t;
        typedef Pair_impl < String_t >        Pair_t;
        typedef typename Value_t::Object      Object_t;
        typedef typename Value_t::Array       Array_t;
        typedef typename String_t::value_type Char_t;

        Semantic_actions( Value_t& value )
        :   value_( value )
        ,   current_p_( 0 )
        {
        }

        void begin_obj( Char_t c )
        {
            assert( c == '{' );

            begin_compound< Object_t >();
        }

        void end_obj( Char_t c )
        {
            assert( c == '}' );

            end_compound();
        }

        void begin_array( Char_t c )
        {
            assert( c == '[' );
     
            begin_compound< Array_t >();
       }

        void end_array( Char_t c )
        {
            assert( c == ']' );

            end_compound();
        }

        void new_name( Iter_t begin, Iter_t end )
        {
            assert( current_p_->type() == obj_type );

            name_ = get_str< String_t >( begin, end );
        }

        void new_str( Iter_t begin, Iter_t end )
        {
            add_to_current( get_str< String_t >( begin, end ) );
        }

        void new_true( Iter_t begin, Iter_t end )
        {
            assert( is_eq( begin, end, "true" ) );

            add_to_current( true );
        }

        void new_false( Iter_t begin, Iter_t end )
        {
            assert( is_eq( begin, end, "false" ) );

            add_to_current( false );
        }

        void new_null( Iter_t begin, Iter_t end )
        {
            assert( is_eq( begin, end, "null" ) );

            add_to_current( Value_t() );
        }

        void new_int( int64_t i )
        {
            add_to_current( i );
        }

        void new_real( double d )
        {
            add_to_current( d );
        }

    private:

        void add_first( const Value_t& value )
        {
            assert( current_p_ == 0 );

            value_ = value;
            current_p_ = &value_;
        }

        template< class Array_or_obj >
        void begin_compound()
        {
            if( current_p_ == 0 )
            {
                add_first( Array_or_obj() );
            }
            else
            {
                stack_.push_back( current_p_ );

                Array_or_obj new_array_or_obj;   // avoid copy by building new array or object in place

                add_to_current( new_array_or_obj );

                if( current_p_->type() == array_type )
                {
                    current_p_ = &current_p_->get_array().back(); 
                }
                else
                {
                    current_p_ = &current_p_->get_obj().back().value_; 
                }
            }
        }

        void end_compound()
        {
            if( current_p_ != &value_ )
            {
                current_p_ = stack_.back();
                
                stack_.pop_back();
            }    
        }

        void add_to_current( const Value_t& value )
        {
            if( current_p_ == 0 )
            {
                add_first( value );
            }
            else if( current_p_->type() == array_type )
            {
                current_p_->get_array().push_back( value );
            }
            else  if( current_p_->type() == obj_type )
            {
                current_p_->get_obj().push_back( Pair_t( name_, value ) );
            }
        }

        Value_t& value_;             // this is the object or array that is being created
        Value_t* current_p_;         // the child object or array that is currently being constructed

        vector< Value_t* > stack_;   // previous child objects and arrays

        String_t name_;              // of current name/value pair
    };

    template< typename Iter_t >
    void throw_error( position_iterator< Iter_t > i, const std::string& reason )
    {
        throw Error_position( i.get_position().line, i.get_position().column, reason );
    }

    template< typename Iter_t >
    void throw_error( Iter_t i, const std::string& reason )
    {
       throw reason;
    }

    // the spirit grammer 
    //
    template< class String_t, class Iter_t >
    class Json_grammer : public grammar< Json_grammer< String_t, Iter_t > >
    {
    public:

        typedef Semantic_actions< String_t, Iter_t > Semantic_actions_t;

        Json_grammer( Semantic_actions_t& semantic_actions )
        :   actions_( semantic_actions )
        {
        }

        static void throw_not_value( Iter_t begin, Iter_t end )
        {
    	    throw_error( begin, "not a value" );
        }

        static void throw_not_array( Iter_t begin, Iter_t end )
        {
    	    throw_error( begin, "not an array" );
        }

        static void throw_not_object( Iter_t begin, Iter_t end )
        {
    	    throw_error( begin, "not an object" );
        }

        static void throw_not_pair( Iter_t begin, Iter_t end )
        {
    	    throw_error( begin, "not a pair" );
        }

        static void throw_not_colon( Iter_t begin, Iter_t end )
        {
    	    throw_error( begin, "no colon in pair" );
        }

        static void throw_not_string( Iter_t begin, Iter_t end )
        {
    	    throw_error( begin, "not a string" );
        }

        template< typename ScannerT >
        struct definition
        {
            definition( const Json_grammer& self )
            {
                typedef typename String_t::value_type Char_t;

                // first we convert the semantic action class methods to functors with the 
                // parameter signature expected by spirit

                typedef function< void( Char_t )         > Char_action;
                typedef function< void( Iter_t, Iter_t ) > Str_action;
                typedef function< void( double )         > Real_action;
                typedef function< void( int64_t )        > Int_action;

                Char_action begin_obj  ( bind( &Semantic_actions_t::begin_obj,   &self.actions_, _1 ) );
                Char_action end_obj    ( bind( &Semantic_actions_t::end_obj,     &self.actions_, _1 ) );
                Char_action begin_array( bind( &Semantic_actions_t::begin_array, &self.actions_, _1 ) );
                Char_action end_array  ( bind( &Semantic_actions_t::end_array,   &self.actions_, _1 ) );
                Str_action  new_name   ( bind( &Semantic_actions_t::new_name,    &self.actions_, _1, _2 ) );
                Str_action  new_str    ( bind( &Semantic_actions_t::new_str,     &self.actions_, _1, _2 ) );
                Str_action  new_true   ( bind( &Semantic_actions_t::new_true,    &self.actions_, _1, _2 ) );
                Str_action  new_false  ( bind( &Semantic_actions_t::new_false,   &self.actions_, _1, _2 ) );
                Str_action  new_null   ( bind( &Semantic_actions_t::new_null,    &self.actions_, _1, _2 ) );
                Real_action new_real   ( bind( &Semantic_actions_t::new_real,    &self.actions_, _1 ) );
                Int_action  new_int    ( bind( &Semantic_actions_t::new_int,     &self.actions_, _1 ) );

                // actual grammer

                json_
                    = value_ | eps_p[ &throw_not_value ]
                    ;

                value_
                    = string_[ new_str ] 
                    | number_ 
                    | object_ 
                    | array_ 
                    | str_p( "true" ) [ new_true  ] 
                    | str_p( "false" )[ new_false ] 
                    | str_p( "null" ) [ new_null  ]
                    ;

                object_ 
                    = ch_p('{')[ begin_obj ]
                    >> !members_
                    >> ( ch_p('}')[ end_obj ] | eps_p[ &throw_not_object ] )
                    ;

                members_
                    = pair_ >> *( ',' >> pair_ )
                    ;

                pair_
                    = string_[ new_name ]
                    >> ( ':' | eps_p[ &throw_not_colon ] )
                    >> ( value_ | eps_p[ &throw_not_value ] )
                    ;

                array_
                    = ch_p('[')[ begin_array ]
                    >> !elements_
                    >> ( ch_p(']')[ end_array ] | eps_p[ &throw_not_array ] )
                    ;

                elements_
                    = value_ >> *( ',' >> value_ )
                    ;

                string_ 
                    = lexeme_d // this causes white space inside a string to be retained
                      [
                          confix_p
                          ( 
                              '"', 
                              *lex_escape_ch_p,
                              '"'
                          ) 
                      ]
                    ;

                number_
                    = strict_real_p[ new_real ] 
                    | int64_p      [ new_int  ]
                    ;
            }

            rule< ScannerT > json_, object_, members_, pair_, array_, elements_, value_, string_, number_;

            const rule< ScannerT >& start() const { return json_; }
        };

        Semantic_actions_t& actions_;
    };

    template< class Iter_t, class Value_t >
    Iter_t read_range_or_throw( Iter_t begin, Iter_t end, Value_t& value )
    {
        typedef typename Value_t::String_type String_t;

        Semantic_actions< String_t, Iter_t > semantic_actions( value );
     
        const parse_info< Iter_t > info = parse( begin, end, 
                                                 Json_grammer< String_t, Iter_t >( semantic_actions ), 
                                                 space_p );

        if( !info.hit )
        {
            assert( false ); // in theory exception should already have been thrown
            throw_error( info.stop, "error" );
        }

        return info.stop;
    }

    template< class Iter_t, class Value_t >
    void add_posn_iter_and_read_range_or_throw( Iter_t begin, Iter_t end, Value_t& value )
    {
        typedef position_iterator< Iter_t > Posn_iter_t;

        const Posn_iter_t posn_begin( begin, end );
        const Posn_iter_t posn_end( end, end );
     
        read_range_or_throw( posn_begin, posn_end, value );
    }

    template< class Iter_t, class Value_t >
    bool read_range( Iter_t& begin, Iter_t end, Value_t& value )
    {
        try
        {
            begin = read_range_or_throw( begin, end, value );

            return true;
        }
        catch( ... )
        {
            return false;
        }
    }

    template< class String_t, class Value_t >
    void read_string_or_throw( const String_t& s, Value_t& value )
    {
        add_posn_iter_and_read_range_or_throw( s.begin(), s.end(), value );
    }

    template< class String_t, class Value_t >
    bool read_string( const String_t& s, Value_t& value )
    {
        typename String_t::const_iterator begin = s.begin();

        return read_range( begin, s.end(), value );
    }

    template< class Istream_t >
    struct Multi_pass_iters
    {
        typedef typename Istream_t::char_type Char_t;
        typedef istream_iterator< Char_t, Char_t > istream_iter;
        typedef multi_pass< istream_iter > multi_pass_iter;

        Multi_pass_iters( Istream_t& is )
        {
            is.unsetf( ios::skipws );

            begin_ = make_multi_pass( istream_iter( is ) );
            end_   = make_multi_pass( istream_iter() );
        }

        multi_pass_iter begin_;
        multi_pass_iter end_;
    };

    template< class Istream_t, class Value_t >
    bool read_stream( Istream_t& is, Value_t& value )
    {
        Multi_pass_iters< Istream_t > mp_iters( is );

        return read_range( mp_iters.begin_, mp_iters.end_, value );
    }

    template< class Istream_t, class Value_t >
    void read_stream_or_throw( Istream_t& is, Value_t& value )
    {
        const Multi_pass_iters< Istream_t > mp_iters( is );

        add_posn_iter_and_read_range_or_throw( mp_iters.begin_, mp_iters.end_, value );
    }
}

bool json_spirit::read( const std::string& s, Value& value )
{
    return read_string( s, value );
}

void json_spirit::read_or_throw( const std::string& s, Value& value )
{
    read_string_or_throw( s, value );
}

bool json_spirit::read( std::istream& is, Value& value )
{
    return read_stream( is, value );
}

void json_spirit::read_or_throw( std::istream& is, Value& value )
{
    read_stream_or_throw( is, value );
}

bool json_spirit::read( std::string::const_iterator& begin, std::string::const_iterator end, Value& value )
{
    return read_range( begin, end, value );
}

void json_spirit::read_or_throw( std::string::const_iterator& begin, std::string::const_iterator end, Value& value )
{
    begin = read_range_or_throw( begin, end, value );
}

#ifndef BOOST_NO_STD_WSTRING

bool json_spirit::read( const std::wstring& s, wValue& value )
{
    return read_string( s, value );
}

void json_spirit::read_or_throw( const std::wstring& s, wValue& value )
{
    read_string_or_throw( s, value );
}

bool json_spirit::read( std::wistream& is, wValue& value )
{
    return read_stream( is, value );
}

void json_spirit::read_or_throw( std::wistream& is, wValue& value )
{
    read_stream_or_throw( is, value );
}

bool json_spirit::read( std::wstring::const_iterator& begin, std::wstring::const_iterator end, wValue& value )
{
    return read_range( begin, end, value );
}

void json_spirit::read_or_throw( std::wstring::const_iterator& begin, std::wstring::const_iterator end, wValue& value )
{
    begin = read_range_or_throw( begin, end, value );
}
#endif
