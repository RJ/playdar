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

#ifndef __PDL_PLATFORM_H__
#define __PDL_PLATFORM_H__

#include <string>

#if defined( WIN32 ) || defined( _WIN32 )

/**
 * @brief Windows platform
 */
#define PLATFORM_WIN32 1

#define PDL_DECL_EXPORT __declspec( dllexport )

#ifdef PDL_SHARED
#ifdef PDL_EXPORT
#define PDL_DECL PDL_DECL_EXPORT
#else
#define PDL_DECL __declspec( dllimport )
#endif
#else
#define PDL_DECL
#endif

/// Char types
#include <tchar.h>
#define PDL_CHAR   char

#elif defined( unix ) || defined( __unix__ )

/**
 * @brief POSIX platform
 */
#define PLATFORM_POSIX 1

#define PDL_DECL_EXPORT
#define PDL_DECL

/// Char types
#define PDL_CHAR   char

#else

/**
 * @brief Unknown platform. Cannot build
 */
#if defined( _MSC_VER )
#pragma error( "Unknown platform" )
#elif defined ( __GNUC__ )
#error( "Unknown platform" )
#endif

#endif

namespace PDL { typedef std::basic_string< PDL_CHAR > pdl_string; }

#endif // __PDL_PLATFORM_H__
