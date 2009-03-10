#ifndef __RS_demo_DL_H__
#define __RS_demo_DL_H__

#include <iostream>
#include <map>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp>

#include "json_spirit/json_spirit.h"

#include "playdar/playdar_plugin_include.h"

namespace playdar {
namespace resolvers {

class demo : public ResolverService
{
public:
    demo(){}
    
    void init(playdar::Config * c, Resolver * r);

    void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    
    std::string name() const { return "Demo"; }
    
protected:    
    ~demo() throw() { };

};

EXPORT_DYNAMIC_CLASS( demo )

}}
#endif
