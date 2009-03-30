/**
 * @mainpage PDL (Portable Dynamic Loader)
 *
 * @section Introduction
 *
 * PDL is a simple and lightweight C++ library, which provides common 
 * cross-platform interface for creating and using dynamically loaded classes.
 *
 * Project home:@n
 * http://sourceforge.net/projects/pdl-cpp/ @n
 * Project downloads:@n
 * http://sourceforge.net/project/showfiles.php?group_id=203000 @n
 *
 *
 * @section Example
 *
 * Dynamically loaded class interface:
 * @code
 * class MyTestInterface : public PDL::DynamicClass
 * {
 * public:
 *	/// Class functionality
 *	virtual void DoSomething() throw() = 0;
 *	/// Declare this class as dynamically loaded
 *	DECLARE_DYNAMIC_CLASS( MyTestInterface )
 * };
 * @endcode
 *
 * Dynamically loaded class implementation:
 * @code
 * class MyTestClass1 : public MyTestInterface
 * {
 * public:
 *	/// Class functionality
 * 	void DoSomething() throw()
 * 	{
 * 		// Do something
 * 	}
 * };
 * @endcode
 *
 * Dynamically loaded class usage:
 * @code
 * try
 * {
 * 	PDL::DynamicLoader & dynamicLoader = PDL::DynamicLoader::Instance();
 * 	MyTestInterface * instance =
 * 		dynamicLoader.GetClassInstance< MyTestInterface >( "./MyTestClass.so", "MyTestClass" );
 * 	instance -> DoSomething();
 * }
 * catch( PDL::LoaderException & ex )
 * {
 * 	fprintf( stderr, "Loader exception: %s\n", ex.what() );
 * }
 * @endcode
 *
 *
 * @section Status
 *
 * The library is under development and testing now.
 *
 *
 * @author Igor Semenov <igor@progz.ru>
 *
 * @version 0.0.1
 *
 * @par License:
 * Copyright (c) 2007-2008, Igor Semenov @n
 * All rights reserved. @n
 * @n
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: @n
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer. @n
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution. @n
 *     * Neither the name of Igor Semenov nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission. @n
 * @n
 * THIS SOFTWARE IS PROVIDED BY IGOR SEMENOV ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL IGOR SEMENOV BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. @n
 * @n
 */

#ifndef __PDL_DYNAMIC_LOADER_HPP__
#define __PDL_DYNAMIC_LOADER_HPP__

#include <platform.h>
#include <memory>

/**
 * @namespace PDL
 * @brief Portable Dynamic Loader
 */
namespace PDL
{

/// FWD
class DynamicClass;
class DynamicLibraryManager;

/**
 * @class DynamicLoader DynamicLoader.hpp <DynamicLoader.hpp>
 * @brief Dynamic loader
 */
class PDL_DECL DynamicLoader
{

public:
	/**
	 * @brief Create class instance
	 * @param libName - [in] library file name
	 * @param className - [in] class name
	 * @return class instance, 0 if failed
	 * Class should be derived from DynamicClass
	 */
	template< typename Class >
	Class * GetClassInstance( const PDL_CHAR * libName, const PDL_CHAR * className )
	{
		return reinterpret_cast< Class * >( GetDynamicInstance( libName, className ) );
	}
	
	/**
	 * @brief Reset dynamic loader
	 * Unload all loaded libraries and free instances
	 */
	void Reset();

	/**
	 * @brief Get dynamic loader instance
	 * @return dynamic loader instance
	 */
	static DynamicLoader & Instance();

	/**
	 * @brief Destructor
	 */
	~DynamicLoader() throw();

private:
	/**
	 * @brief Constructor
	 */
	DynamicLoader();

	/**
	 * @brief Create class instance
	 * @param libName - [in] library file name
	 * @param className - [in] class name
	 * @return class instance, 0 if failed
	 */
	DynamicClass * GetDynamicInstance( const PDL_CHAR * libName, const PDL_CHAR * className );
	
	/// Forbid copy constructor and assignment operator
	DynamicLoader( const DynamicLoader & );
	DynamicLoader & operator= ( const DynamicLoader & );

private:
	/// Library manager
	std::auto_ptr< DynamicLibraryManager > libraryManager_;

}; // class PDL_DECL DynamicLoader

} // namespace PDL

#endif // __PDL_DYNAMIC_CLASS_FACTORY_HPP__
