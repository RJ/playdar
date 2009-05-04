// Boost uuid.hpp header file  ----------------------------------------------//

// Copyright 2007 Andy Tompkins.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  05 Dec 2008 - Initial Revision (Move from uuid.hpp)
//  05 Dec 2008 - Switch from unsigned long to uint32_t (mt19937's result_type)
//  05 Dec 2008 - Made endian independant

#ifndef BOOST_UUID_RANDOM_GENERATOR_HPP
#define BOOST_UUID_RANDOM_GENERATOR_HPP

#include <boost/uuid/uuid.hpp>

#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/uuid/detail/seed_rng.hpp>
#include <boost/random/detail/ptr_helper.hpp>

namespace boost {
namespace uuids {

template <typename UniformRandomNumberGenerator = mt19937>
class random_generator
{
private:
    typedef boost::uniform_int<uint32_t> distribution_type;
    typedef random::detail::ptr_helper<UniformRandomNumberGenerator> helper_type;
    
    typedef boost::variate_generator<typename helper_type::reference_type, distribution_type> v_gen_type;

public:
    typedef uuid result_type;

    explicit random_generator(UniformRandomNumberGenerator rng)
        : _rng(rng)
        , _v_gen(helper_type::ref(_rng),
                 distribution_type(
                     (std::numeric_limits<uint32_t>::min)(),
                     (std::numeric_limits<uint32_t>::max)())
                 )
    {
        // don't seed generators that are passed in
        //detail::seed<helper_type::value_type>(helper_type::ref(_rng));
    }

    random_generator()
        : _v_gen(helper_type::ref(_rng),
                 distribution_type(
                     (std::numeric_limits<uint32_t>::min)(),
                     (std::numeric_limits<uint32_t>::max)())
                 )
    {
        // seed generator with good values
        detail::seed<typename helper_type::value_type>(helper_type::ref(_rng));
    }

    static uuid::variant_type variant() { return uuid::variant_rfc_4122; }
    static uuid::version_type version() { return uuid::version_random; }

    uuid operator()()
    {
        unsigned char data[16];

        for (std::size_t i = 0; i < 4; ++i)
        {
            uint32_t new_rand = _v_gen();
            data[i*4+0] = ((new_rand >> 24) & 0xFF);
            data[i*4+1] = ((new_rand >> 16) & 0xFF);
            data[i*4+2] = ((new_rand >> 8) & 0xFF);
            data[i*4+3] = ((new_rand >> 0) & 0xFF);
        }

        // set variant
        // should be 0b10xxxxxx
        data[8] &= 0xBF;
        data[8] |= 0x80;

        // set version
        // should be 0b0100xxxx
        data[6] &= 0x4F; //0b01001111
        data[6] |= 0x40; //0b01000000

        uuid u;
        u.assign(data, data+16);
        return u;
    }

private:
    // note these must be in this order so that _rng is created before _v_gen
    UniformRandomNumberGenerator _rng;
    v_gen_type _v_gen;
};

}} //namespace boost::uuids

#endif

