#ifndef __RS_DEMO_H__
#define __RS_DEMO_H__
// All resolver plugins should include this header:
#include "playdar/playdar_plugin_include.h"

namespace playdar {
namespace resolvers {

class demo : public ResolverPlugin<demo>
{
public:
    demo() {}
    
    /** you should call the base implementation if you reimplement */
    virtual bool init( pa_ptr );

    /** the base version is pure virtual */
    virtual void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    
    /** current convention dictates not appending "Resolver" */
    std::string name() const { return "Demo"; }
    
    void Destroy()
    {
        std::cout << "DTOR(destroy) Demo" << std::endl;
    }

protected:
    virtual ~demo() throw() {}
    
private:
    pa_ptr m_pap;
};

// This makes the plugin loading possible:
EXPORT_DYNAMIC_CLASS( demo )

} // resolvers
} // playdar
#endif
