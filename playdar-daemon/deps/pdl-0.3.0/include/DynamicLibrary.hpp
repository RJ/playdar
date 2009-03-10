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

#ifndef __PDL_DYNAMIC_LIBRARY_HPP__
#define __PDL_DYNAMIC_LIBRARY_HPP__

#include <platform.h>
#include <string>
#include <map>

#if PLATFORM_WIN32
#include <windows.h>
#endif

/**
 * @namespace PDL
 * @brief Portable Dynamic Loader
 */
namespace PDL
{

/// FWD
class DynamicClass;

/**
 * @class DynamicLibrary DynamicLibrary.hpp <DynamicLibrary.hpp>
 * @brief Platform-specific dynamic library
 */
class DynamicLibrary
{

public:
	/**
	 * @brief Constructor
	 */
	DynamicLibrary();

	/**
	 * @brief Destructor
	 */
	~DynamicLibrary() throw();

	/**
	 * @brief Open library
	 * @param libName - [in] library file name
	 * @return true - loaded successfully, false otherwise
	 */
	bool Open( const PDL_CHAR * libName, bool resolveSymbols = true );

	/**
	 * @brief Get default library filename extention (platform-specific)
	 * @return default library filename extention
	 */
	static const PDL_CHAR * GetDefaultExt();

	/**
	 * @brief Check if library is opened
	 * @return true if library is opened, false otherwise
	 */
	bool Opened() const { return ( library_ ); }
	
	/**
	 * @brief Get class instance
	 * @param className - [in] class name
	 * @return pointer to class instance
	 */
	DynamicClass * GetInstance( const PDL_CHAR * className );

	/**
	 * @brief Get last error description
	 * @return last error description
	 */
	pdl_string GetLastError() const;

	/**
	 * @brief Close library
	 * @return true if closed successfully, false otherwise
	 */
	bool Close();

private:
	/**
	 * @brief Get symbol by name
	 * @param symbolName - [in] symbol name
	 * @return pointer to symbol, 0 if not found
	 */
	void * GetSymbolByName( const PDL_CHAR * symbolName );
	
	/// Forbid copy constructor and assignment operator
	DynamicLibrary( const DynamicLibrary & );
	DynamicLibrary & operator=( const DynamicLibrary & );

private:
	// Library handle
#if PLATFORM_WIN32
	HMODULE library_;
#elif PLATFORM_POSIX
	void * library_;
#endif

	// Library name
	pdl_string libraryName_;

	// Symbol map
	typedef std::map< pdl_string, DynamicClass * > InstanceMap;
	InstanceMap instances_;

}; // class DynamicLibrary

} // namespace PDL

#endif // __PDL_DYNAMIC_LIBRARY_HPP__
