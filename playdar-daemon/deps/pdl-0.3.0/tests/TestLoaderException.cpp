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

#include <LoaderException.hpp>
#include "UnitTest.hpp"

void throwing_func()
{
	throw PDL::LoaderException( "Some text" );
}
 
int main( int argc, char ** argv )
{
	// Test catching PDL::LoaderException
	try
	{
		throwing_func();
		UNIT_TEST( false );
	}
	catch( PDL::LoaderException & ex )
	{
		fprintf( stderr, "OK: PDL::LoaderException catched: %s\n", ex.what() );
		UNIT_TEST( true );
	}
	catch( ... )
	{
		UNIT_TEST( false );
	}
	
	// Test catching std::exception
	try
	{
		throwing_func();
		UNIT_TEST( false );
	}
	catch( std::exception & ex )
	{
		fprintf( stderr, "OK: std::exception catched: %s\n", ex.what() );
		UNIT_TEST( true );
	}
	catch( ... )
	{
		UNIT_TEST( false );
	}

	return 0;
}
