//  (C) Copyright Andy Tompkins 2007. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Purpose: to test serializing uuids with narrow archives

#include <boost/test/included/test_exec_monitor.hpp>
#include <boost/test/test_tools.hpp>
#include <sstream>
#include <iostream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_serialize.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/lexical_cast.hpp>

template <class OArchiveType, class IArchiveType, class OStringStreamType, class IStringStreamType>
void test_archive()
{
    using namespace std;
    using namespace boost::uuids;
    
    OStringStreamType o_stream;
    
    uuid u1 = boost::lexical_cast<uuid>("12345678-90ab-cdef-1234-567890abcdef");

    uuid u2 = boost::uuids::nil();
    
    // save
    {
        OArchiveType oa(o_stream);
        
        oa << BOOST_SERIALIZATION_NVP(u1);
    }
    
    //cout << "stream:" << o_stream.str() << "\n\n";
    
    // load
    {
        IStringStreamType i_stream(o_stream.str());
        IArchiveType ia(i_stream);
        
        ia >> BOOST_SERIALIZATION_NVP(u2);
    }
    
    BOOST_CHECK_EQUAL(u1, u2);
}

int test_main( int /* argc */, char* /* argv */[] )
{
    using namespace std;
    using namespace boost::archive;
    
    test_archive<text_oarchive, text_iarchive, ostringstream, istringstream>();
    test_archive<xml_oarchive, xml_iarchive, ostringstream, istringstream>();
    test_archive<binary_oarchive, binary_iarchive, ostringstream, istringstream>();

    return 0;
}
