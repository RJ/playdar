// Boost uuid_serialize.hpp header file  ------------------------------------//

// Copyright 2007 Andy Tompkins.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  12 Nov 2007 - Initial Revision
//  25 Feb 2008 - moved to namespace boost::uuids::detail

#ifndef BOOST_UUID_SERIALIZE_HPP
#define BOOST_UUID_SERIALIZE_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/serialization/level.hpp>
//#include <boost/serialization/tracking.hpp>
//#include <boost/serialization/split_free.hpp>
//#include <boost/serialization/nvp.hpp>
//#include <boost/serialization/access.hpp>

BOOST_CLASS_IMPLEMENTATION(boost::uuids::uuid, boost::serialization::primitive_type)

//BOOST_SERIALIZATION_SPLIT_FREE(boost::uuids::uuid)

//namespace boost {
//namespace serialization {

//template <class Archive>
//inline void save(Archive& ar, boost::uuids::uuid const& u, const unsigned int /*file_version*/)
//{
//  uuid::const_iterator end = u.end();
//  for (uuid::const_iterator begin = u.begin(); begin!=end; ++begin) {
//      //ar & boost::serialization::make_nvp("item", (*begin));
//      ar & (*begin);
//  }
//}

//template <class Archive>
//inline void load(Archive& ar, boost::uuids::uuid& u, const unsigned int /*file_version*/)
//{
//  boost::uuids::uuid::value_type data[16];
//  for (int i=0; i<16; ++i) {
//      //ar & boost::serialization::make_nvp("item", data[i]);
//      ar & data[i];
//  }
//  u = boost::uuids::uuid(&data[0], &data[0]+16);
//}

//} // namespace serialization
//} // namespace boost

#endif // BOOST_UUID_SERIALIZE_HPP
