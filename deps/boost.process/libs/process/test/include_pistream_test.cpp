// 
// Boost.Process 
// ~~~~~~~~~~~~~ 
// 
// Copyright (c) 2006, 2007 Julio M. Merino Vidal 
// Copyright (c) 2008 Boris Schaeling 
// 
// Distributed under the Boost Software License, Version 1.0. (See accompanying 
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt) 
// 

#include <boost/process/pistream.hpp> 

namespace bp = ::boost::process; 
namespace bpd = ::boost::process::detail; 

bp::pistream *test_it() 
{ 
    bpd::file_handle fh; 
    return new bp::pistream(fh); 
} 
