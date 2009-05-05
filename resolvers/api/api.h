#ifndef __RS_API_H__
#define __RS_API_H__
// All resolver plugins should include this header:
#include "playdar/playdar_plugin_include.h"

namespace playdar {
namespace resolvers {

class api : public ResolverPlugin<api>
{
public:
    api() {}
    
    bool init( pa_ptr );
    void start_resolving(boost::shared_ptr<ResolverQuery> rq) {} //noop
    unsigned short weight() const { return 0; }
    unsigned short targettime() const { return 0; }
    std::string name() const { return "api"; }

    // all we really do is handle http requests:
    playdar_response anon_http_handler(const playdar_request*);
    playdar_response authed_http_handler(const playdar_request*, playdar::auth* pauth);
    
protected:
    virtual ~api() throw() {}
    
private:
    pa_ptr m_pap;
};

// This makes the plugin loading possible:
EXPORT_DYNAMIC_CLASS( api )

} // resolvers
} // playdar
#endif
