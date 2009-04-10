// boost/uuid/sha1.hpp header file  ----------------------------------------------//

// Copyright 2007 Andy Tompkins.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  29 May 2007 - Initial Revision
//  25 Feb 2008 - moved to namespace boost::uuids::detail
//  05 Dec 2008 - Updated to handle larger messages (up to size_t)
//  05 Dec 2008 - Updated to handle non-32-bit ints

// This is a byte oriented implementation

#ifndef BOOST_UUID_DETAIL_SHA1_H
#define BOOST_UUID_DETAIL_SHA1_H

#include <boost/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>

#include <climits>
#include <cstddef>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std {
    using ::size_t;
} // namespace std
#endif

namespace boost {
namespace uuids {
namespace detail {

BOOST_STATIC_ASSERT(CHAR_BIT == 8);
BOOST_STATIC_ASSERT(sizeof(int)*8 >= 32); // For constants

inline uint32_t left_rotate(uint32_t x, std::size_t n)
{
    return (x<<n) ^ (x>> (32-n));
}

class sha1
{
public:
    typedef unsigned int(&digest_type)[5];
public:
    sha1();

    void reset();

    void process_byte(uint8_t byte);
    template <typename ByteInputIterator>
    void process_range(ByteInputIterator begin_iter, ByteInputIterator end_iter);
    void process_block(void const* bytes_begin, void const* bytes_end);
    void process_bytes(void const* buffer, std::size_t byte_count);

    void get_digest(digest_type digest);

private:
    void process_block();

private:
    uint32_t h_[5];

    uint8_t block_[64];

    std::size_t block_byte_index_;
    std::size_t byte_count_;
};

inline sha1::sha1()
{
    reset();
}

inline void sha1::reset()
{
    h_[0] = 0x67452301u;
    h_[1] = 0xEFCDAB89u;
    h_[2] = 0x98BADCFEu;
    h_[3] = 0x10325476u;
    h_[4] = 0xC3D2E1F0u;

    block_byte_index_ = 0;
    byte_count_ = 0;
}

inline void sha1::process_byte(uint8_t byte)
{
    block_[block_byte_index_++] = byte;
    ++byte_count_;
    if (block_byte_index_ == 64) {
        block_byte_index_ = 0;
        process_block();
    }
}

template <typename ByteInputIterator>
inline void sha1::process_range(ByteInputIterator begin_iter, ByteInputIterator end_iter) 
{
    while (begin_iter != end_iter) {
        process_byte(*begin_iter++);
    }
}

inline void sha1::process_block(void const* bytes_begin, void const* bytes_end)
{
    unsigned char const* begin = static_cast<unsigned char const*>(bytes_begin);
    unsigned char const* end = static_cast<unsigned char const*>(bytes_end);
    process_range(begin, end);
}

inline void sha1::process_bytes(void const* buffer, std::size_t byte_count)
{
    unsigned char const* b = static_cast<unsigned char const*>(buffer);
    process_block(b, b+byte_count);
}

inline void sha1::process_block()
{
    unsigned int w[80];
    for (std::size_t i=0; i<16; ++i) {
        w[i]  = (block_[i*4 + 0] << 24);
        w[i] |= (block_[i*4 + 1] << 16);
        w[i] |= (block_[i*4 + 2] << 8);
        w[i] |= (block_[i*4 + 3]);
    }
    for (std::size_t i=16; i<80; ++i) {
        w[i] = left_rotate((w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]), 1);
    }

    uint32_t a = h_[0];
    uint32_t b = h_[1];
    uint32_t c = h_[2];
    uint32_t d = h_[3];
    uint32_t e = h_[4];

    for (std::size_t i=0; i<80; ++i) {
        uint32_t f;
        uint32_t k;

        if (i<20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999u;
        } else if (i<40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1u;
        } else if (i<60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCu;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        uint32_t temp = left_rotate(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = left_rotate(b, 30);
        b = a;
        a = temp;
    }

    h_[0] += a;
    h_[1] += b;
    h_[2] += c;
    h_[3] += d;
    h_[4] += e;
}

inline void sha1::get_digest(digest_type digest)
{
    std::size_t bit_count = byte_count_*8;
    BOOST_ASSERT(bit_count >= byte_count_);

    // append the bit '1' to the message
    process_byte(0x80);

    // append k bits '0', where k is the minimum number >= 0
    // such that the resulting message length is congruent to 56 (mod 64)
    // check if there is enough space for padding and bit_count
    if (block_byte_index_ > 56) {
        // finish this block
        while (block_byte_index_ != 0) {
            process_byte(0);
        }

        // one more block
        while (block_byte_index_ < 56) {
            process_byte(0);
        }
    } else {
        while (block_byte_index_ < 56) {
            process_byte(0);
        }
    }

    // append length of message (before pre-processing) 
    // as a 64-bit big-endian integer
    uint8_t length_data[8];
    std::size_t bc = bit_count;
    for (std::size_t i = 8; i--; bc >>= 8) {
        length_data[i] = static_cast<unsigned char>(bc & 0xFF);
    }
    for (std::size_t i = 0; i < 8; ++i) {
        process_byte(length_data[i]);
    }

    // get final digest
    digest[0] = h_[0];
    digest[1] = h_[1];
    digest[2] = h_[2];
    digest[3] = h_[3];
    digest[4] = h_[4];
}


}}} // namespace boost::uuids::detail

#endif
