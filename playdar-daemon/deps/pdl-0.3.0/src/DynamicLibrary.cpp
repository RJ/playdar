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
#include <string>
#include <cstring> //memcpy

#if PLATFORM_WIN32
#include <windows.h>
#elif PLATFORM_POSIX
#include <dlfcn.h>
#endif

/**
 * @namespace PDL
 * @brief Portable Dynamic Loader
 */
namespace PDL
{

/**
 * @brief Dynamic instance builder
 */
typedef DynamicClass * ( ( * DynamicBuilder )() );

/**
 * @brief Constructor
 */
DynamicLibrary::DynamicLibrary() : library_( 0 ), libraryName_(), instances_()
{
	;;
}

/**
 * @brief Destructor
 */
DynamicLibrary::~DynamicLibrary() throw()
{
	( void ) Close();
}
	
/**
 * @brief Open library
 * @param libName - [in] library file name
 * @return true - loaded successfully, false otherwise
 */
bool DynamicLibrary::Open( const PDL_CHAR * libName, bool resolveSymbols )
{
	if ( !libName ) { return false; }

	library_ =
#if PLATFORM_WIN32
		::LoadLibraryExA( libName, NULL,
		                  ( resolveSymbols ) ? 0 : DONT_RESOLVE_DLL_REFERENCES );
#elif PLATFORM_POSIX
		dlopen( libName, RTLD_GLOBAL | ( ( resolveSymbols ) ? RTLD_NOW : RTLD_LAZY ) );
#endif

	if ( library_ ) { libraryName_ = libName; }
	else
	{
		const pdl_string nameWithExt( pdl_string( libName ) + pdl_string( GetDefaultExt() ) );
		library_ =
#if PLATFORM_WIN32
			::LoadLibraryExA( nameWithExt.c_str(), NULL,
		                      ( resolveSymbols ) ? 0 : DONT_RESOLVE_DLL_REFERENCES );
#elif PLATFORM_POSIX
			dlopen( nameWithExt.c_str(), RTLD_GLOBAL | ( ( resolveSymbols ) ? RTLD_NOW : RTLD_LAZY ) );
#endif
		if ( library_ ) { libraryName_ = libName; }
	}

	return ( library_ );
}

/**
 * @brief Get default library filename extention (platform-specific)
 * @return default library filename extention
 */
const PDL_CHAR * DynamicLibrary::GetDefaultExt()
{
#if PLATFORM_WIN32
	return ".dll";
#elif defined( hpux ) || defined( _hpux ) || defined( __hpux )
	return ".sl";
#elif PLATFORM_POSIX
	return ".so";
#endif
}

/**
 * @brief Get class instance
 * @param className - [in] class name
 * @return pointer to class instance
 */
DynamicClass * DynamicLibrary::GetInstance( const PDL_CHAR * className )
{
	const InstanceMap::iterator loadedInstance( instances_.find( className ) );
	if ( loadedInstance != instances_.end() ) { return ( loadedInstance -> second ); }
	
	const pdl_string builderName( pdl_string( "Create" ) + pdl_string( className ) );
	
	// Tricky code to prevent compiler error
	// ISO C++ forbids casting between pointer-to-function and pointer-to-object
	DynamicBuilder builder = 0;
	void * symbol = GetSymbolByName( builderName.c_str() );
	( void ) memcpy( &builder, &symbol, sizeof( void * ) );
	
	if ( !builder )
	{
		instances_[ className ] = 0;
		throw LoaderException( pdl_string( "Class `" ) + pdl_string( className ) +
		                       pdl_string( "` not found in " )  + libraryName_ );
	}

	DynamicClass * instance = ( DynamicClass * )( *builder )();
	instances_[ className ] = instance;
	return instance;
}

/**
 * @brief Get symbol by name
 * @param symbolName - [in] symbol name
 * @return pointer to symbol, 0 if not found
 */
void * DynamicLibrary::GetSymbolByName( const PDL_CHAR * symbolName )
{
	if ( !library_ )
	{
		throw LoaderException( pdl_string( "Library is not opened" ) );
	}

	return
#if PLATFORM_WIN32
		static_cast< void * >( ::GetProcAddress( library_, symbolName ) );
#elif PLATFORM_POSIX
		dlsym( library_, symbolName );
#endif
}

/**
 * @brief Get last error description
 * @return last error description
 */
pdl_string DynamicLibrary::GetLastError() const
{
	try
	{
#if PLATFORM_WIN32
		LPSTR lpMsgBuf = NULL;
		const DWORD res =
			::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			                  NULL, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	                          reinterpret_cast< LPSTR >( &lpMsgBuf ), 0, NULL );
		pdl_string errorText;
		if ( res != 0 )
		{
			errorText.assign( lpMsgBuf );
			( void ) ::LocalFree( lpMsgBuf );
		}
#elif PLATFORM_POSIX
		const pdl_string errorText( dlerror() );
#endif
		return errorText;
	}
	catch ( ... )
	{
		return pdl_string();
	}
}

/**
 * @brief Close library
 * @return true if closed successfully, false otherwise
 */
bool DynamicLibrary::Close()
{
	const InstanceMap::iterator end( instances_.end() );
	for ( InstanceMap::iterator i( instances_.begin() ); i != end; ++i )
	{
		( i -> second ) -> Destroy();
	}
	instances_.clear();

	bool closeSuccess = true;
	if ( library_ )
	{
		closeSuccess = 
#if PLATFORM_WIN32
			( ::FreeLibrary( library_ ) != FALSE );
#elif PLATFORM_POSIX
			( dlclose( library_ ) == 0 );
#endif
		library_ = 0;
	}
	libraryName_.clear();

	return closeSuccess;
}

} // namespace PDL
