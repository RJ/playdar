// Boost uuid.hpp header file  ----------------------------------------------//

// Copyright 2006 Scott McMurray.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  5 Dec 2008 - Initial Revision

#ifndef BOOST_UUID_HPP
#define BOOST_UUID_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_namespaces.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/sha1_generator.hpp>
#ifdef BOOST_HAS_E2FSPROGS
#include <boost/uuid/e2fsprogs_generator.hpp>
#endif
#ifdef BOOST_HAS_WINDOWS // FIXME
#include <boost/uuid/windows_generator.hpp>
#endif

#endif

