#ifndef JASON_SPIRIT_TEST_UTILS
#define JASON_SPIRIT_TEST_UTILS

/* Copyright (c) 2007-2009 John W Wilkinson

   This source code can be used for any purpose as long as
   this comment is retained. */

// json spirit version 3.00

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <cassert>

// these functions allow you to inspect the values that caused a test to fail

template< class T1, class T2 >
void assert_eq( const T1& t1, const T2& t2 )
{
    if( t1 == t2 ) return;

    assert( false );
}

template< class T1, class T2 >
void assert_neq( const T1& t1, const T2& t2 )
{
    if( !(t1 == t2) ) return;

    assert( false );
}

#endif
