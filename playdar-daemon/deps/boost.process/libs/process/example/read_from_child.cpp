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

#include <boost/process.hpp> 
#include <string> 
#include <vector> 
#include <iostream> 

namespace bp = ::boost::process; 

bp::child start_child() 
{ 
    std::string exec = "bjam"; 

    std::vector<std::string> args; 
    args.push_back("bjam"); 
    args.push_back("--version"); 

    bp::context ctx; 
    ctx.stdout_behavior = bp::capture_stream(); 

    return bp::launch(exec, args, ctx); 
} 

int main() 
{ 
    bp::child c = start_child(); 

    bp::pistream &is = c.get_stdout(); 
    std::string line; 
    while (std::getline(is, line)) 
        std::cout << line << std::endl; 
} 
