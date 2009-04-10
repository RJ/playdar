// Boost e2fsprogs_generator.hpp header file  -------------------------------//

// Copyright 2008 Scott McMurray
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  05 Dec 2008 - Initial Revision

// If you use (not just include) this, then you need to link with libuuid

#ifndef BOOST_UUID_E2FSPROGS_GENERATOR_HPP
#define BOOST_UUID_E2FSPROGS_GENERATOR_HPP

#include <boost/uuid/uuid.hpp>

#include <uuid/uuid.h>

// uuid_t is a char[16], so passing &*u.begin() is acceptable
BOOST_STATIC_ASSERT(sizeof(uuid_t) == 16);

namespace boost {
namespace uuids {

struct e2fsprogs_default {
    void generate(uuid::value_type *u) const { uuid_generate(u); }
};
struct e2fsprogs_random {
    void generate(uuid_t u) const { uuid_generate_random(u); }
};
struct e2fsprogs_time {
    void generate(uuid_t u) const { uuid_generate_time(u); }
};

#ifndef BOOST_UUID_DEFAULT_E2FSPROGS_GENERATOR
#define BOOST_UUID_DEFAULT_E2FSPROGS_GENERATOR e2fsprogs_default
#endif

template <typename Strategy = BOOST_UUID_DEFAULT_E2FSPROGS_GENERATOR>
class e2fsprogs_generator : Strategy
{
public:
    typedef uuid result_type;

    uuid operator()()
    {
        uuid u;
        Strategy::generate(&*u.begin());
        return u;
    }
};

#ifndef BOOST_UUID_NATIVE_GENERATOR_DEFINED
#define BOOST_UUID_NATIVE_GENERATOR_DEFINED
typedef e2fsprogs_generator<> native_generator;
#endif

}} //namespace boost::uuids

#endif

