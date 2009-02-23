/* Copyright (c) 2007-2009 John W Wilkinson

   This source code can be used for any purpose as long as
   this comment is retained. */

// json spirit version 3.00

#include "json_spirit.h"
#include <cassert>
#include <algorithm>
#include <fstream>
#include <boost/bind.hpp>

using namespace std;
using namespace boost;
using namespace json_spirit;

void add_pair( Object& obj, const string& name, const Value& value )
{
    obj.push_back( Pair( name, value ) );
}

// adds a new child object to an existing object,
// returns the new child object
//
Object& add_child_obj( Object& parent, const string& name )
{
    parent.push_back( Pair( name, Object() ) );

    return parent.back().value_.get_obj();
}

// adds a new child object to an existing array,
// returns the new child object
//
Object& add_child_obj( Array& parent )
{
    parent.push_back( Object() );

    return parent.back().get_obj();
}

Array& add_array( Object& obj, const string& name )
{
    obj.push_back( Pair( name, Array() ) );

    return obj.back().value_.get_array();
}

bool same_name( const Pair& pair, const string& name )
{
    return pair.name_ == name;
}

const Value& find_value( const Object& obj, const string& name )
{
    Object::const_iterator i = find_if( obj.begin(), obj.end(), bind( same_name, _1, ref( name ) ) );

    if( i == obj.end() ) return Value::null;

    return i->value_;
}

const Array& find_array( const Object& obj, const string& name )
{
    return find_value( obj, name ).get_array(); 
}

int find_int( const Object& obj, const string& name )
{
    return find_value( obj, name ).get_int();
}

string find_str( const Object& obj, const string& name )
{
    return find_value( obj, name ).get_str();
}

struct Address
{
    bool operator==( const Address& lhs ) const
    {
        return ( house_number_ == lhs.house_number_ ) &&
               ( road_         == lhs.road_ ) &&
               ( town_         == lhs.town_ ) &&
               ( county_       == lhs.county_ ) &&
               ( country_      == lhs.country_ );
    }

    int house_number_;
    string road_;
    string town_;
    string county_;
    string country_;
};

void write_address( Array& a, const Address& addr )
{
    Object& addr_obj( add_child_obj( a ) );

    add_pair( addr_obj, "house_number", addr.house_number_ );
    add_pair( addr_obj, "road",         addr.road_ );
    add_pair( addr_obj, "town",         addr.town_ );
    add_pair( addr_obj, "county",       addr.county_ );
    add_pair( addr_obj, "country",      addr.country_ );

    // an alternative method to do the above would be as below,
    // but the push_back causes a copy of the object to made
    // which could be expensive for complex objects
    //
    //Object addr_obj;

    //add_pair( addr_obj, "house_number", addr.house_number_ );
    //add_pair( addr_obj, "road",         addr.road_ );
    //add_pair( addr_obj, "town",         addr.town_ );
    //add_pair( addr_obj, "county",       addr.county_ );
    //add_pair( addr_obj, "country",      addr.country_ );

    //a.push_back( addr_obj );
}

Address read_address( const Object& obj )
{
    Address addr;

    addr.house_number_ = find_int( obj, "house_number" );
    addr.road_         = find_str( obj, "road" );        
    addr.town_         = find_str( obj, "town" );        
    addr.county_       = find_str( obj, "county" );       
    addr.country_      = find_str( obj, "country" );      

    return addr;
}

void write_addrs( const char* file_name, const Address addrs[] )
{
    Object root_obj;

    Array& addr_array( add_array( root_obj, "addresses" ) );

    for( int i = 0; i < 5; ++i )
    {
        write_address( addr_array, addrs[i] );
    }

    ofstream os( file_name );

    write_formatted( root_obj, os );

    os.close();
}

vector< Address > read_addrs( const char* file_name )
{
    ifstream is( file_name );

    Value value;

    read( is, value );

    Object root_obj( value.get_obj() );  // a 'value' read from a stream could be an JSON array or object

    const Array& addr_array( find_array( root_obj, "addresses" ) );

    vector< Address > addrs;

    for( unsigned int i = 0; i < addr_array.size(); ++i )
    {
        addrs.push_back( read_address( addr_array[i].get_obj() ) );
    }

    return addrs;
}

int main()
{
    const Address addrs[5] = { { 42, "East Street",  "Newtown",     "Essex",        "England" },
                               { 1,  "West Street",  "Hull",        "Yorkshire",    "England" },
                               { 12, "South Road",   "Aberystwyth", "Dyfed",        "Wales"   },
                               { 45, "North Road",   "Paignton",    "Devon",        "England" },
                               { 78, "Upper Street", "Ware",        "Hertforshire", "England" } };

    const char* file_name( "demo.txt" );

    write_addrs( file_name, addrs );

    vector< Address > new_addrs( read_addrs( file_name ) );

    assert( new_addrs.size() == 5 );

    for( int i = 0; i < 5; ++i )
    {
        assert( new_addrs[i] == addrs[i] );
    }

	return 0;
}
