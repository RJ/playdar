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

#ifndef BOOST_UUID_UUID_NAMESPACES_HPP
#define BOOST_UUID_UUID_NAMESPACES_HPP

#include <boost/uuid/uuid.hpp>

namespace boost {
namespace uuids {

// Predefined UUIDS

// RFC 4122, Appendix C
namespace namespaces {

/* Name string is a fully-qualified domain name */
uuid dns() {
    uuid u = { /* 6ba7b810-9dad-11d1-80b4-00c04fd430c8 */
       0x6b, 0xa7, 0xb8, 0x10,
       0x9d, 0xad,
       0x11, 0xd1,
       0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8
    };
    return u;
}

/* Name string is a URL */
uuid url() {
    uuid u = { /* 6ba7b811-9dad-11d1-80b4-00c04fd430c8 */
       0x6b, 0xa7, 0xb8, 0x11,
       0x9d, 0xad,
       0x11, 0xd1,
       0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8
    };
    return u;
}

/* Name string is an ISO OID */
uuid oid() {
    uuid u = { /* 6ba7b812-9dad-11d1-80b4-00c04fd430c8 */
       0x6b, 0xa7, 0xb8, 0x12,
       0x9d, 0xad,
       0x11, 0xd1,
       0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8
    };
    return u;
}

/* Name string is an X.500 DN (in DER or a text output format) */
uuid x500() {
    uuid u = { /* 6ba7b814-9dad-11d1-80b4-00c04fd430c8 */
       0x6b, 0xa7, 0xb8, 0x14,
       0x9d, 0xad,
       0x11, 0xd1,
       0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8
    };
    return u;
}

} //namespace namespaces

} //namespace uuids

} //namespace boost

#endif // BOOST_UUID_HPP
