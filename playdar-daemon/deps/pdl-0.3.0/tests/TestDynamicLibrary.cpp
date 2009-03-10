/**
 * Copyright (c) 2007-2008, Igor Semenov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Igor Semenov nor the names of its contributors 
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY IGOR SEMENOV ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL IGOR SEMENOV BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <DynamicClass.hpp>
#include <DynamicLibrary.hpp>
#include <LoaderException.hpp>
#include "UnitTest.hpp"

int main( int argc, char ** argv )
{
	if ( argc != 2 )
	{
		fprintf( stderr, "ERROR: Usage %s <libName>\n", argv[ 0 ] );\
		return -1;
	}

	PDL::DynamicLibrary lib1;
	PDL::DynamicLibrary lib2;
	PDL::DynamicLibrary lib3;
	
	// Test DynamicLibrary::Open()
	UNIT_TEST( !lib1.Open( "SomeDefinitelyNotExistentFile" ) );
	fprintf( stderr, "LastError: (%s)\n", lib1.GetLastError().c_str() );

	UNIT_TEST( lib2.Open( argv[ 1 ] ) );
	fprintf( stderr, "LastError: (%s)\n", lib2.GetLastError().c_str() );
	
	UNIT_TEST( !lib3.Open( 0 ) );
	fprintf( stderr, "LastError: (%s)\n", lib3.GetLastError().c_str() );
	
	// Test DynamicLibrary::Opened()
	UNIT_TEST( !lib1.Opened() );
	UNIT_TEST( lib2.Opened() );
	
	// Test DynamicLibrary::GetInstance()
	try
	{
		lib1.GetInstance( "MyTestClass1" );
		UNIT_TEST( false );
	}
	catch( PDL::LoaderException & ex )
	{
		fprintf( stderr, "Exception: %s, LastError: (%s)\n", ex.what(), lib1.GetLastError().c_str() );
		UNIT_TEST( true );
	}
	catch( ... )
	{
		UNIT_TEST( false );
	}
	
	try
	{
		lib2.GetInstance( "NonExistentClass" );
		UNIT_TEST( false );
	}
	catch( PDL::LoaderException & ex )
	{
		fprintf( stderr, "Exception: %s, LastError: (%s)\n", ex.what(), lib2.GetLastError().c_str() );
		UNIT_TEST( true );
	}
	catch( ... )
	{
		UNIT_TEST( false );
	}
	
	try
	{
		lib2.GetInstance( "MyTestClass1" );
		lib2.GetInstance( "MyTestClass2" );
		UNIT_TEST( true );
	}
	catch( PDL::LoaderException & ex )
	{
		fprintf( stderr, "Exception: %s, LastError: (%s)\n", ex.what(), lib2.GetLastError().c_str() );
		UNIT_TEST( false );
	}
	catch( ... )
	{
		UNIT_TEST( false );
	}
	
	try
	{
		PDL::DynamicClass * class1 = lib2.GetInstance( "MyTestClass1" );
		PDL::DynamicClass * class2 = lib2.GetInstance( "MyTestClass1" );
		UNIT_TEST( ( class1 ) && ( class2 ) && ( class1 == class2 ) );
	}
	catch( PDL::LoaderException & ex )
	{
		fprintf( stderr, "Exception: %s, LastError: (%s)\n", ex.what(), lib2.GetLastError().c_str() );
		UNIT_TEST( false );
	}
	catch( ... )
	{
		UNIT_TEST( false );
	}

	// Test DynamicLibrary::Close()
	UNIT_TEST( lib1.Close() );
	UNIT_TEST( lib2.Close() );

	return 0;
}
