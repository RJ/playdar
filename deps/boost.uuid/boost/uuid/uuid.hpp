// Boost uuid.hpp header file  ----------------------------------------------//

// Copyright 2006 Andy Tompkins.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  06 Feb 2006 - Initial Revision
//  09 Nov 2006 - fixed variant and version bits for v4 guids
//  13 Nov 2006 - added serialization
//  17 Nov 2006 - added name-based guid creation
//  20 Nov 2006 - add fixes for gcc (from Tim Blechmann)
//  07 Mar 2007 - converted to header only
//  10 May 2007 - removed need for Boost.Thread
//              - added better seed - thanks Peter Dimov
//              - removed null()
//              - replaced byte_count() and output_bytes() with size() and begin() and end()
//  11 May 2007 - fixed guid(ByteInputIterator first, ByteInputIterator last)
//              - optimized operator>>
//  14 May 2007 - converted from guid to uuid
//  29 May 2007 - uses new implementation of sha1
//  01 Jun 2007 - removed using namespace directives
//  09 Nov 2007 - moved implementation to uuid.ipp file
//  12 Nov 2007 - moved serialize code to uuid_serialize.hpp file
//  25 Feb 2008 - moved to namespace boost::uuids
//  05 Dec 2008 - moved stream IO to io.hpp
//  05 Dec 2008 - made an aggregate (removed boost.operators, made members public)
//  05 Dec 2008 - moved implementation back from uuid.ipp file

#pragma once
#ifndef BOOST_UUID_UUID_HPP
#define BOOST_UUID_UUID_HPP

#include <boost/array.hpp>
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/static_assert.hpp>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std {
    using ::size_t
} //namespace std
#endif

namespace boost {
namespace uuids {

class uuid
{
    typedef array<uint8_t, 16> data_type;

public:
    typedef data_type::value_type value_type;
    typedef data_type::iterator iterator;
    typedef data_type::const_iterator const_iterator;
    typedef data_type::difference_type difference_type;
    typedef data_type::size_type size_type;

public:

    template <typename ByteInputIterator>
    ByteInputIterator assign(ByteInputIterator first, ByteInputIterator last)
    {
        data_type::iterator i_data = data_.begin();
        for (size_type i = 0; i < 16; ++i) {
            BOOST_ASSERT( first != last );
            *i_data++ = numeric_cast<uint8_t>(*first++);
        }
        return first;
    }

    bool is_nil() const {
        for (size_type i = 0; i < 16; ++i) {
            if (data_[i]) return false;
        }
        return true;
    }
    typedef bool (uuid::*unspecified_bool_type)() const;
    operator unspecified_bool_type() const 
    {
        return is_nil() ? 0 : &uuid::is_nil;
    }
    bool operator!() const /* throw() */
    {
        return is_nil();
    }

    static size_type size() { return data_type::size(); } /* throw() */
    iterator begin() { return data_.begin(); } /* throw() */
    iterator end() { return data_.end(); } /* throw() */
    const_iterator begin() const { return data_.begin(); } /* throw() */
    const_iterator end() const { return data_.end(); } /* throw() */

    enum variant_type {variant_ncs = 0,
                       variant_dce = 1, 
                       variant_rfc_4122 = variant_dce,
                       variant_microsoft = 2, 
                       variant_other = 3}; 
    variant_type variant() const
    {
        value_type b = data_[8];
        // 0b 0xxx xxxx
        if ( (b & 0x80) == 0x00 ) return variant_ncs;
        // 0b 10xx xxxx
        if ( (b & 0xC0) == 0x80 ) return variant_dce;
        // 0b 110x xxxx
        if ( (b & 0xE0) == 0xC0 ) return variant_microsoft;
        // 0b 111x xxxx
        BOOST_ASSERT( (b & 0xE0) == 0xE0 );
        return variant_other;
    }

    enum version_type {version_wrong_variant = -1,
                       version_unknown = 0,
                       version_1 = 1,
                       version_time_based = version_1,
                       version_2 = 2,
                       version_dce_security = version_2,
                       version_3 = 3,
                       version_name_based_md5 = version_3,
                       version_4 = 4,
                       version_random = version_4,
                       version_5 = 5,
                       version_name_based_sha1 = version_5};
    version_type version() const
    {
        if (variant() != variant_dce) {
            return version_wrong_variant;
        }
        
        value_type v = data_[6] >> 4;
        if (1 <= v && v <= 5) {
            return version_type(v);
        }

        return version_unknown;
    }

    void swap(uuid &rhs) /* throw() */
    {
        std::swap(data_, rhs.data_);
    }

public:
    data_type data_; /* Exposition Only */
};

inline void swap(uuid &x, uuid &y)
{
    x.swap(y);
}

bool operator==(uuid const& lhs, uuid const& rhs) /* throw() */
{
    return (lhs.data_ == rhs.data_);
}
bool operator!=(uuid const& lhs, uuid const& rhs) /* throw() */
{
    return !(lhs == rhs);
}

bool operator<(uuid const& lhs, uuid const& rhs) /* throw() */
{
    return (lhs.data_ < rhs.data_);
}
bool operator>(uuid const& lhs, uuid const& rhs) /* throw() */
{
    return rhs < lhs;
}
bool operator<=(uuid const& lhs, uuid const& rhs) /* throw() */
{
    return !(lhs > rhs);
}
bool operator>=(uuid const& lhs, uuid const& rhs) /* throw() */
{
    return !(lhs < rhs);
}

// This is equivalent to boost::hash_range(u.begin(), u.end());
inline std::size_t hash_value(uuid const& u)
{
    std::size_t seed = 0;
    for(uuid::const_iterator i=u.begin(); i != u.end(); ++i)
    {
        seed ^= static_cast<std::size_t>(*i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    return seed;
}

// As named in RFC 4122, 4.1.7 
uuid nil() {
    uuid u = uuid();
    return u;
}

} //namespace uuids

// Nobody wants to type boost::uuids::uuid
using uuids::uuid;

} //namespace boost

#endif // BOOST_UUID_HPP
