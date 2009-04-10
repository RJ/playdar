// Boost io.hpp header file  ------------------------------------------------//

// Copyright 2007 Andy Tompkins.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  5 Dec 2008 - Initial Revision (Move from uuid.hpp)

#ifndef BOOST_UUID_IO_HPP
#define BOOST_UUID_IO_HPP

#include <boost/uuid/uuid.hpp>

#include <boost/io/ios_state.hpp>
#include <iosfwd>
#include <locale>

namespace boost {
namespace uuids {

namespace detail {

int get_showbraces_index()
{
    static int index = std::ios_base::xalloc();
    return index;
}

} // namespace detail

inline bool get_showbraces(std::ios_base & iosbase) {
    return (iosbase.iword(detail::get_showbraces_index()) != 0);
}

inline void set_showbraces(std::ios_base & iosbase, bool showbraces) {
    iosbase.iword(detail::get_showbraces_index()) = showbraces;
}

inline std::ios_base& showbraces(std::ios_base& iosbase)
{
    set_showbraces(iosbase, true);
    return iosbase;
}

inline std::ios_base& noshowbraces(std::ios_base& iosbase)
{
    set_showbraces(iosbase, false);
    return iosbase;
}

template <typename ch, typename char_traits>
    std::basic_ostream<ch, char_traits>& operator<<(std::basic_ostream<ch, char_traits> &os, uuid const& u)
{
    io::ios_flags_saver flags_saver(os);
    io::ios_width_saver width_saver(os);
    io::basic_ios_fill_saver<ch, char_traits> fill_saver(os);

    const typename std::basic_ostream<ch, char_traits>::sentry ok(os);
    if (ok) {
        bool showbraces = get_showbraces(os);
        if (showbraces) {
            os << os.widen('{');
        }
        os << std::hex;
        os.fill(os.widen('0'));

        std::size_t i=0;
        for (uuid::const_iterator i_data = u.begin(); i_data!=u.end(); ++i_data, ++i) {
            os.width(2);
            os << static_cast<unsigned int>(*i_data);
            if (i == 3 || i == 5 || i == 7 || i == 9) {
                os << os.widen('-');
            }
        }
        if (showbraces) {
            os << os.widen('}');
        }
    }
    return os;
}

template <typename ch, typename char_traits>
    std::basic_istream<ch, char_traits>& operator>>(std::basic_istream<ch, char_traits> &is, uuid &u)
{
    const typename std::basic_istream<ch, char_traits>::sentry ok(is);
    if (ok) {
        unsigned char data[16];

        typedef std::ctype<ch> ctype_t;
        ctype_t const& ctype = std::use_facet<ctype_t>(is.getloc());

        ch xdigits[16];
        {
            char szdigits[17] = "0123456789ABCDEF";
            ctype.widen(szdigits, szdigits+16, xdigits);
        }
        ch*const xdigits_end = xdigits+16;

        ch c;
        c = is.peek();
        bool bHaveBraces = false;
        if (c == is.widen('{')) {
            bHaveBraces = true;
            is >> c; // read brace
        }

        for (std::size_t i=0; i<u.size() && is; ++i) {
            is >> c;
            c = ctype.toupper(c);

            ch* f = std::find(xdigits, xdigits_end, c);
            if (f == xdigits_end) {
                is.setstate(std::ios_base::failbit);
                break;
            }

            unsigned char byte = static_cast<unsigned char>(std::distance(&xdigits[0], f));

            is >> c;
            c = ctype.toupper(c);
            f = std::find(xdigits, xdigits_end, c);
            if (f == xdigits_end) {
                is.setstate(std::ios_base::failbit);
                break;
            }

            byte <<= 4;
            byte |= static_cast<unsigned char>(std::distance(&xdigits[0], f));

            data[i] = byte;

            if (is) {
                if (i == 3 || i == 5 || i == 7 || i == 9) {
                    is >> c;
                    if (c != is.widen('-')) is.setstate(std::ios_base::failbit);
                }
            }
        }

        if (bHaveBraces && is) {
            is >> c;
            if (c != is.widen('}')) is.setstate(std::ios_base::failbit);
        }
        if (is) {
            u.assign(data, data+16);
        }
    }
    return is;
}

}} // namespace boost::uuids

#endif

