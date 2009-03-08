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

#ifndef __PDL_LOADER_EXCEPTION_HPP__
#define __PDL_LOADER_EXCEPTION_HPP__

#include <platform.h>
#include <exception>
#include <string>

/**
 * @namespace PDL
 * @brief Portable Dynamic Loader
 */
namespace PDL
{

/**
 * @class LoaderException LoaderException.hpp <LoaderException.hpp>
 * @brief Dynamic loader exception class
 */
class PDL_DECL LoaderException: public std::exception
{

public:
	/**
	 * @brief Constructor
	 * @param text - [in] exception description
	 */
	LoaderException( const pdl_string & text );
	
	/**
	 * @brief Destructor
	 */
	~LoaderException() throw();

	/**
	 * @brief Get exception description
	 * @return exception description
	 */
	const char * what() const throw();

private:
	// Exception description
	const pdl_string text_;

}; // class PDL_DECL LoaderException

} // namespace PDL

#endif // __PDL_LOADER_EXCEPTION_HPP__
