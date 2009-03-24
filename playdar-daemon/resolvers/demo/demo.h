#ifndef __RS_DEMO_H__
#define __RS_DEMO_H__
// All resolver plugins should include this header:
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
    
    void Destroy()
    {
        cout << "DTOR(destroy) Demo" << endl;
    }
protected:    
    ~demo() throw() { };

};

// This makes the plugin loading possible:
EXPORT_DYNAMIC_CLASS( demo )

} // resolvers
} // playdar
#endif
