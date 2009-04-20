// Boost windows_generator.hpp header file  -------------------------------//

// Copyright 2008 Scott McMurray
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  05 Dec 2008 - Initial Revision

/******************************************************************************

                        WARNING! WARNING! WARNING!

                This is absolutely, 100%, completely untested.

 ******************************************************************************/

#ifndef BOOST_UUID_WINDOWS_GENERATOR_HPP
#define BOOST_UUID_WINDOWS_GENERATOR_HPP

#include <boost/uuid/uuid.hpp>

#include <Rpc.h>

// uuid_t is a char[16], so passing &*u.begin() is acceptable
BOOST_STATIC_ASSERT(sizeof(uuid_t) == 16);

namespace boost {
namespace uuids {

class windows_generator
{
public:
    typedef uuid result_type;

    uuid operator()()
    {
        UUID u_;
        UuidCreate(&u_);
        char *p = reinterpret_cast<char*>(&u_);
        uuid u;
        u.assign(p, p + sizeof(UUID));
        return u;
    }
};

#ifndef BOOST_UUID_NATIVE_GENERATOR_DEFINED
#define BOOST_UUID_NATIVE_GENERATOR_DEFINED
typedef windows_generator<> native_generator;
#endif

}} //namespace boost::uuids

#endif

