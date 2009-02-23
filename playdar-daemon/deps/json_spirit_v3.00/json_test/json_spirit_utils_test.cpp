/* Copyright (c) 2007-2009 John W Wilkinson

   This source code can be used for any purpose as long as
   this comment is retained. */

// json spirit version 3.00

#include "json_spirit_utils_test.h"
#include "json_spirit_utils.h"
#include "utils_test.h"

#include <boost/assign/list_of.hpp>

using namespace json_spirit;
using namespace std;
using namespace boost::assign;

namespace
{
    template< class Obj_t, class Map_t >
    struct Test_runner
    {
        typedef typename Map_t::key_type    String_t;
        typedef typename Obj_t::value_type  Pair_t;
        typedef typename Pair_t::Value_type Value_t;

        String_t to_str( const char* c_str )
        {
            return ::to_str< String_t >( c_str );
        }
 
        void assert_equal( const Obj_t& obj, const Map_t& mp_obj )
        {
            typename Obj_t::size_type obj_size = obj.size();
            typename Map_t::size_type map_size = mp_obj.size();

            assert_eq( obj_size, map_size );

            for( typename Obj_t::const_iterator i = obj.begin(); i != obj.end(); ++i )
            {
                assert_eq( mp_obj.find( i->name_ )->second, i->value_ );
            }
        }

        void check_obj_to_map( const Obj_t& obj )
        {
            Map_t mp_obj;

            obj_to_map( obj, mp_obj );

            assert_equal( obj, mp_obj );
        }

        void check_map_cleared()
        {
            Map_t mp_obj;

            mp_obj[ to_str( "del" ) ] = to_str( "me" );

            obj_to_map( Obj_t(), mp_obj );

            assert_eq( mp_obj.size(), 0u );
        }

        void check_map_to_obj( const Map_t& mp_obj )
        {
            Obj_t obj;

            map_to_obj( mp_obj, obj );

            assert_equal( obj, mp_obj );
        }

        void check_obj_cleared()
        {
            Obj_t obj;

            obj.push_back( Pair_t( to_str( "del" ), to_str( "me" ) ) );

            map_to_obj( Map_t(), obj );

            assert_eq( obj.size(), 0u );
        }

        void test_obj_to_map()
        {
            check_obj_to_map( Obj_t() );
            check_obj_to_map( list_of( Pair_t( to_str( "a" ), 1 ) ) );
            check_obj_to_map( list_of( Pair_t( to_str( "a" ), 1 ) )( Pair_t( to_str( "b" ), 2 ) ) );
            check_obj_to_map( list_of( Pair_t( to_str( "a" ), 1 ) )( Pair_t( to_str( "b" ), 2 ) )( Pair_t( to_str( "c" ), 3 ) ) );
            check_map_cleared();

            check_map_to_obj( Map_t() );
            check_map_to_obj( map_list_of( to_str( "a" ), 1 ) );
            check_map_to_obj( map_list_of( to_str( "a" ), 1 )( to_str( "b" ), 2 ) );
            check_map_to_obj( map_list_of( to_str( "a" ), 1 )( to_str( "b" ), 2 )( to_str( "c" ), 3 ) );
            check_obj_cleared();
        }

        void check_find( const Obj_t& obj, const char* name, const Value_t& expected_result )
        {
            const Value_t& result = find_value( obj, to_str( name ) );

            assert_eq( result, expected_result );
        }

        void test_find()
        {
            check_find( Obj_t(), "not there", Value_t::null );

            const Obj_t obj_1 = list_of( Pair_t( to_str( "a" ), 1 ) );

            check_find( obj_1, "not there", Value_t::null );
            check_find( obj_1, "a", 1 );

            const Obj_t obj_2 = list_of( Pair_t( to_str( "a" ), 1 ) )( Pair_t( to_str( "ab" ), 2 ) );

            check_find( obj_2, "a", 1 );
            check_find( obj_2, "ab", 2 );

            const Obj_t obj_3 = list_of( Pair_t( to_str( "a" ), 1 ) )( Pair_t( to_str( "ab" ), 2 ) )( Pair_t( to_str( "abc" ), 3 ) );

            check_find( obj_3, "a", 1 );
            check_find( obj_3, "ab", 2 );
            check_find( obj_3, "abc", 3 );
        }

        void run_tests()
        {
            test_obj_to_map();
            test_find();
        }
    };
}

void json_spirit::test_utils()
{
    Test_runner< Object, Mapped_obj >().run_tests();

#ifndef BOOST_NO_STD_WSTRING
    Test_runner< wObject, wMapped_obj >().run_tests();
#endif
}
