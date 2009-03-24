#ifndef __RS_DEMO_H__
#define __RS_DEMO_H__
// All resolver plugins should include this header:
#include "playdar/playdar_plugin_include.h"

namespace playdar {
namespace resolvers {

class demo : public ResolverService
{
public:
    demo() {}
    
    /** you should call the base implementation if you reimplement */
    virtual void init(playdar::Config * c, Resolver * r);

    /** the base version is pure virtual */
    virtual void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    
    /** current convention dictates not appending "Resolver" */
    std::string name() const { return "Demo"; }
    
    void Destroy()
    {
        cout << "DTOR(destroy) Demo" << endl;
    }

protected:
    virtual ~demo() throw() {}
};

// This makes the plugin loading possible:
EXPORT_DYNAMIC_CLASS( demo )

} // resolvers
} // playdar
#endif
