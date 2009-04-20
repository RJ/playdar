// Boost sha1_generator.hpp header file  ------------------------------------//

// Copyright 2007 Andy Tompkins.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  5 Dec 2008 - Initial Revision (Move from uuid.hpp, uuid.ipp)

#ifndef BOOST_UUID_SHA1_GENERATOR_HPP
#define BOOST_UUID_SHA1_GENERATOR_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_namespaces.hpp>

#include <boost/uuid/detail/sha1.hpp>
#include <boost/generator_iterator.hpp>

namespace boost {
namespace uuids {

class sha1_generator
{
public:
    typedef uuid result_type;

    explicit sha1_generator(uuid namespace_uuid)
        : namespace_uuid_(namespace_uuid) {}

    static uuid::variant_type variant() { return uuid::variant_rfc_4122; }
    static uuid::version_type version() { return uuid::version_name_based_sha1; }

    template <typename ByteInputIterator>
    uuid operator()(ByteInputIterator begin, ByteInputIterator end)
    {
        detail::sha1 sha;
        sha.process_range(namespace_uuid_.begin(), namespace_uuid_.end());
        sha.process_range(begin, end);
        unsigned int digest[5];

        sha.get_digest(digest);

        unsigned char data[16];
        for (int i=0; i<4; ++i) {
            data[i*4+0] = ((digest[i] >> 24) & 0xFF);
            data[i*4+1] = ((digest[i] >> 16) & 0xFF);
            data[i*4+2] = ((digest[i] >> 8) & 0xFF);
            data[i*4+3] = ((digest[i] >> 0) & 0xFF);
        }

        // set variant
        // should be 0b10xxxxxx
        data[8] &= 0xBF;
        data[8] |= 0x80;

        // set version
        // should be 0b0101xxxx
        data[6] &= 0x5F; //0b01011111
        data[6] |= 0x50; //0b01010000

        uuid u;
        u.assign(data, data+16);
        return u;

    }

    uuid operator()(char const *data, std::size_t data_length)
    {
        return operator()(data, data+data_length);
    }
    
private:
    uuid namespace_uuid_;
};

}} //namespace boost::uuids

#endif

