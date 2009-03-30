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

#include "./platform.h"
#include <DynamicLibrary.hpp>
#include <DynamicLibraryManager.hpp>
#include <LoaderException.hpp>

/**
 * @namespace PDL
 * @brief Portable Dynamic Loader
 */
namespace PDL
{

/**
 * @brief Constructor
 */
DynamicLibraryManager::DynamicLibraryManager() : libraries_()
{
	;;
}

/**
 * @brief Destructor
 */
DynamicLibraryManager::~DynamicLibraryManager() throw()
{
	Reset();
}

/**
 * @brief Reset library manager
 * Free all libraries and set initial state
 */
void DynamicLibraryManager::Reset()
{
	// Free all libraries
	const DynamicLibraryManager::LibraryMap::iterator end( libraries_.end() );
	for ( DynamicLibraryManager::LibraryMap::iterator i( libraries_.begin() );
	      i != end;
		  ++i )
	{
		DynamicLibrary * library = i -> second;
		if ( library )
		{
			library -> Close();
			delete library;
		}
	}

	// Clear map
	libraries_.clear();
}

/**
 * @brief Get dynamic library
 * @param libName - [in] library file name
 * @return dynamic library
 * @throw LoaderException - cannot load library
 */
DynamicLibrary & DynamicLibraryManager::GetLibrary( const PDL_CHAR * libName )
{
	DynamicLibrary & library = GetLibraryInstance( libName );

	if ( !library.Opened() )
	{
		// We tried to load this library before, but we've failed
		throw LoaderException( pdl_string( "Cannot load library `" ) +
		                       pdl_string( libName ) + pdl_string( "`" ) );
	}

	return library;
}

/**
 * @brief Get dynamic library instance
 * @param libName - [in] library file name
 * @return dynamic library instance
 */
DynamicLibrary & DynamicLibraryManager::GetLibraryInstance( const PDL_CHAR * libName )
{
	if ( !libName )
	{
		throw LoaderException( pdl_string( "Library name is 0" ) );
	}

	DynamicLibrary * library = 0;
	const LibraryMap::iterator libraryInstance( libraries_.find( libName ) );
	if ( libraryInstance != libraries_.end() )
	{
		library = libraryInstance -> second;
	}
	else
	{
		library = new DynamicLibrary();
		libraries_[ libName ] = library;
		if ( ( library ) && ( !library -> Open( libName, true ) ) )
		{
			// This is the first time we try to load this library.
			// We are able to specify the reason of failure.
			throw LoaderException( pdl_string( "Cannot load library `" ) +
			                       pdl_string( libName ) + pdl_string( "`: " ) +
			                       library -> GetLastError() );
		}
	}
	
	if ( !library )
	{
		// Actually, this should never happen.
		// If it has happened, this means serious memory damage
		throw LoaderException( pdl_string( "Internal error" ) );
	}

	return ( *library );
}

} // namespace PDL
