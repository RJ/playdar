//  (C) Copyright Andy Tompkins 2007. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  libs/uuid/test/test_uuid.cpp  -------------------------------//

#include <boost/uuid.hpp>

#include <boost/test/included/test_exec_monitor.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/output_test_stream.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/random.hpp>

#include <boost/functional/hash.hpp>

#include <string>
#include <vector>

int test_main(int, char*[])
{
    using namespace boost::uuids;
    using boost::test_tools::output_test_stream;
    using boost::lexical_cast;
    
    uuid u;

    // test constructors
    BOOST_CHECK_EQUAL(uuid(), uuid());

    BOOST_CHECK_NO_THROW(lexical_cast<uuid>("{00000000-0000-0000-0000-000000000000}"));
    BOOST_CHECK_NO_THROW(lexical_cast<uuid>("00000000-0000-0000-0000-000000000000"));
#ifndef BOOST_NO_STD_WSTRING
    BOOST_CHECK_NO_THROW(lexical_cast<uuid>(L"{00000000-0000-0000-0000-000000000000}"));
#endif

    uuid temp_uuid = uuid();

    std::vector<long> vec;
    vec.push_back(0x12);
    vec.push_back(0x34);
    vec.push_back(0x56);
    vec.push_back(0x78);
    vec.push_back(0x90);
    vec.push_back(0xab);
    vec.push_back(0xcd);
    vec.push_back(0xef);
    vec.push_back(0x12);
    vec.push_back(0x34);
    vec.push_back(0x56);
    vec.push_back(0x78);
    vec.push_back(0x90);
    vec.push_back(0xab);
    vec.push_back(0xcd);
    vec.push_back(0xef);
    BOOST_CHECK_NO_THROW(temp_uuid.assign(vec.begin(), vec.end()));
    BOOST_CHECK_EQUAL(temp_uuid, lexical_cast<uuid>("12345678-90ab-cdef-1234-567890abcdef"));
    
    // test assignment
    uuid u1 = lexical_cast<uuid>("{12345678-90ab-cdef-1234-567890abcdef}");
    temp_uuid = u1;
    BOOST_CHECK_EQUAL(temp_uuid, u1);
    temp_uuid = uuid();
    BOOST_CHECK_EQUAL(temp_uuid, uuid());

    // test operator!
    BOOST_CHECK(!uuid());
    BOOST_CHECK(!temp_uuid);
    BOOST_CHECK(u1);

    // test comparsion
    BOOST_CHECK_PREDICATE(std::equal_to<uuid>(), (u1)(u1));
    BOOST_CHECK_PREDICATE(std::not_equal_to<uuid>(), (u1)(uuid()));
   
    BOOST_CHECK_PREDICATE(std::less<uuid>(), (uuid())(u1));
    BOOST_CHECK_PREDICATE(std::less_equal<uuid>(), (uuid())(u1));
    BOOST_CHECK_PREDICATE(std::less_equal<uuid>(), (u1)(u1));
    BOOST_CHECK_PREDICATE(std::less<uuid>(), (lexical_cast<uuid>("00000000-0000-0000-0000-000000000000")) (lexical_cast<uuid>("00000000-0000-0000-0000-000000000001")));
    BOOST_CHECK_PREDICATE(std::less<uuid>(), (lexical_cast<uuid>("e0000000-0000-0000-0000-000000000000")) (lexical_cast<uuid>("f0000000-0000-0000-0000-000000000000")));
    BOOST_CHECK_PREDICATE(std::less<uuid>(), (lexical_cast<uuid>("ffffffff-ffff-ffff-ffff-fffffffffffe")) (lexical_cast<uuid>("ffffffff-ffff-ffff-ffff-ffffffffffff")));
    
    BOOST_CHECK_PREDICATE(std::greater<uuid>(), (u1)(uuid()));
    BOOST_CHECK_PREDICATE(std::greater_equal<uuid>(), (u1)(uuid()));
    BOOST_CHECK_PREDICATE(std::greater_equal<uuid>(), (u1)(u1));

    // test to_string()
    BOOST_CHECK_EQUAL(lexical_cast<std::string>(uuid()), 
                      std::string("00000000-0000-0000-0000-000000000000"));
    BOOST_CHECK_EQUAL(lexical_cast<std::string>(u1), 
                      std::string("12345678-90ab-cdef-1234-567890abcdef"));

    // test to_wstring()
#ifndef BOOST_NO_STD_WSTRING
    BOOST_CHECK(lexical_cast<std::wstring>(uuid()) ==
                std::wstring(L"00000000-0000-0000-0000-000000000000"));
    BOOST_CHECK(lexical_cast<std::wstring>(u1) ==
                std::wstring(L"12345678-90ab-cdef-1234-567890abcdef"));
#endif

    // test with lexical_cast
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(uuid()), std::string("00000000-0000-0000-0000-000000000000"));
    BOOST_CHECK_EQUAL(boost::lexical_cast<uuid>("00000000-0000-0000-0000-000000000000"), uuid());

    // test size
    BOOST_CHECK_EQUAL(u1.size(), 16U);

    // test begin/end
    vec.clear();
    vec.resize(u1.size());
    std::copy(u1.begin(), u1.end(), vec.begin());
    BOOST_CHECK_EQUAL(vec.size(), 16U);
    BOOST_CHECK_EQUAL(vec[0], 0x12);
    BOOST_CHECK_EQUAL(vec[1], 0x34);
    BOOST_CHECK_EQUAL(vec[2], 0x56);
    BOOST_CHECK_EQUAL(vec[3], 0x78);
    BOOST_CHECK_EQUAL(vec[4], 0x90);
    BOOST_CHECK_EQUAL(vec[5], 0xab);
    BOOST_CHECK_EQUAL(vec[6], 0xcd);
    BOOST_CHECK_EQUAL(vec[7], 0xef);
    BOOST_CHECK_EQUAL(vec[8], 0x12);
    BOOST_CHECK_EQUAL(vec[9], 0x34);
    BOOST_CHECK_EQUAL(vec[10], 0x56);
    BOOST_CHECK_EQUAL(vec[11], 0x78);
    BOOST_CHECK_EQUAL(vec[12], 0x90);
    BOOST_CHECK_EQUAL(vec[13], 0xab);
    BOOST_CHECK_EQUAL(vec[14], 0xcd);
    BOOST_CHECK_EQUAL(vec[15], 0xef);

    // test swap()
    uuid u2 = boost::lexical_cast<uuid>("{abababab-abab-abab-abab-abababababab}");
    BOOST_CHECK_NO_THROW(swap(u2, u1));
    BOOST_CHECK_EQUAL(u1, boost::lexical_cast<uuid>("{abababab-abab-abab-abab-abababababab}"));
    BOOST_CHECK_EQUAL(u2, boost::lexical_cast<uuid>("{12345678-90ab-cdef-1234-567890abcdef}"));

    BOOST_CHECK_NO_THROW(u1.swap(u2));
    BOOST_CHECK_EQUAL(u2, boost::lexical_cast<uuid>("{abababab-abab-abab-abab-abababababab}"));
    BOOST_CHECK_EQUAL(u1, boost::lexical_cast<uuid>("{12345678-90ab-cdef-1234-567890abcdef}"));
    
    // test hash
    boost::hash<uuid> uuid_hasher;
    if (sizeof(std::size_t) == 4) {
        BOOST_CHECK_EQUAL(uuid_hasher(uuid()), 3565488559UL);
        BOOST_CHECK_EQUAL(uuid_hasher(u1), 4159045843UL);
        BOOST_CHECK_EQUAL(uuid_hasher(u2), 2713274306UL);
    } else {
        BOOST_CHECK_EQUAL(uuid_hasher(uuid()), 17562157925649023279UL);
        BOOST_CHECK_EQUAL(uuid_hasher(u1), 6622376135548557651UL);
        BOOST_CHECK_EQUAL(uuid_hasher(u2), 4700797755868797762UL);
    }

    // test insert/extract operators
    output_test_stream output;
    output << uuid();
    BOOST_CHECK(!output.is_empty(false));
    BOOST_CHECK(output.check_length(36, false));
    BOOST_CHECK(output.is_equal("00000000-0000-0000-0000-000000000000"));

    output << u1;
    BOOST_CHECK(!output.is_empty(false));
    BOOST_CHECK(output.check_length(36, false));
    BOOST_CHECK(output.is_equal("12345678-90ab-cdef-1234-567890abcdef"));

    output << showbraces << u1;
    BOOST_CHECK(!output.is_empty(false));
    BOOST_CHECK(output.check_length(38, false));
    BOOST_CHECK(output.is_equal("{12345678-90ab-cdef-1234-567890abcdef}"));

    output << noshowbraces << u1;
    BOOST_CHECK(!output.is_empty(false));
    BOOST_CHECK(output.check_length(36, false));
    BOOST_CHECK(output.is_equal("12345678-90ab-cdef-1234-567890abcdef"));

    std::stringstream ss;
    ss << "{00000000-0000-0000-0000-000000000000}";
    ss >> u1;
    BOOST_CHECK_EQUAL(u1, uuid());

    ss << "{12345678-90ab-cdef-1234-567890abcdef}";
    ss >> u1;
    BOOST_CHECK_EQUAL(u1, lexical_cast<uuid>("{12345678-90ab-cdef-1234-567890abcdef}"));
    
    ss.str("");
    ss << u1;
    BOOST_CHECK_EQUAL(ss.str(), "12345678-90ab-cdef-1234-567890abcdef");

    ss.str("");
    ss << showbraces << u1;
    BOOST_CHECK_EQUAL(ss.str(), "{12345678-90ab-cdef-1234-567890abcdef}");

    std::wstringstream wss;
    wss << L"{00000000-0000-0000-0000-000000000000}";
    wss >> u1;
    BOOST_CHECK_EQUAL(u1, uuid());

    wss << L"{12345678-90ab-cdef-1234-567890abcdef}";
    wss >> u1;
    BOOST_CHECK_EQUAL(u1, lexical_cast<uuid>("{12345678-90ab-cdef-1234-567890abcdef}"));
    BOOST_CHECK_EQUAL(u1, lexical_cast<uuid>(L"{12345678-90ab-cdef-1234-567890abcdef}"));

    wss.str(L"");
    wss << u1;
    BOOST_CHECK(wss.str() == L"12345678-90ab-cdef-1234-567890abcdef");

    wss.str(L"");
    wss << showbraces << u1;
    BOOST_CHECK(wss.str() == L"{12345678-90ab-cdef-1234-567890abcdef}");

    // test basic_uuid_generation
    {
        random_generator<> gen;

        BOOST_CHECK(uuid() != gen());
        temp_uuid = gen();
        BOOST_CHECK(temp_uuid != gen());
    }
    {
        random_generator<boost::rand48> gen;

        BOOST_CHECK(uuid() != gen());
        temp_uuid = gen();
        BOOST_CHECK(temp_uuid != gen());
    }
    {
        boost::lagged_fibonacci44497 rng;
        random_generator<boost::lagged_fibonacci44497&> gen(rng);

        BOOST_CHECK(uuid() != gen());
        temp_uuid = gen();
        BOOST_CHECK(temp_uuid != gen());
    }
    {
        boost::scoped_ptr<boost::mt19937> rng(new boost::mt19937);
        random_generator<boost::mt19937*> gen(rng.get());

        BOOST_CHECK(uuid() != gen());
        temp_uuid = gen();
        BOOST_CHECK(temp_uuid != gen());
    }
    //{
    //    boost_uuid_generator<boost::random_device> gen;

    //    BOOST_CHECK(uuid() != gen());
    //    temp_uuid = gen();
    //    BOOST_CHECK(temp_uuid != gen());
    //}
    
#ifdef BOOST_HAS_E2FSPROGS
    {
        e2fsprogs_generator<> gen;

        BOOST_CHECK(uuid() != gen());
        temp_uuid = gen();
        BOOST_CHECK(temp_uuid != gen());
    }
    {
        e2fsprogs_generator<e2fsprogs_time> gen;

        BOOST_CHECK(uuid() != gen());
        temp_uuid = gen();
        BOOST_CHECK(temp_uuid != gen());
    }
    {
        e2fsprogs_generator<e2fsprogs_random> gen;

        BOOST_CHECK(uuid() != gen());
        temp_uuid = gen();
        BOOST_CHECK(temp_uuid != gen());
    }
#endif
    
    // test variant and version bits
    random_generator<> gen;
    for (int i=0; i<5; ++i) { // check a few generated uuids
        uuid u = gen();
        vec.resize(u.size());
        std::copy(u.begin(), u.end(), vec.begin());
        
        // variant
        BOOST_CHECK_EQUAL(vec[8] & 0xC0, 0x80);
        
        // version
        BOOST_CHECK_EQUAL(vec[6] & 0xF0, 0x40);
    }
    
    // test name based creation
    {
        sha1_generator gen(namespaces::dns());
        BOOST_CHECK(uuid() != gen("www.widgets.com", 15));
        temp_uuid = gen("www.widgets.com", 15);
        BOOST_CHECK(temp_uuid == gen("www.widgets.com", 15));
        BOOST_CHECK_EQUAL(temp_uuid,
                          lexical_cast<uuid>("21f7f8de-8051-5b89-8680-0195ef798b6a"));
        std::string name = "www.widgets.com";
        BOOST_CHECK_EQUAL(temp_uuid, gen(name.begin(), name.end()));
    }
    
    return 0;
}
