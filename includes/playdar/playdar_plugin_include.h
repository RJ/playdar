// all the stuff you need as a resolver plugin:
#ifndef __PLAYDAR_PLUGIN_INCLUDE_H
#define __PLAYDAR_PLUGIN_INCLUDE_H

#include "playdar/resolver_service.h"

namespace playdar {

/// this is what the dynamically loaded resolver plugins extend:
template <typename TPluginType>
class ResolverPlugin 
 : public ResolverServicePlugin
{

private:

    /// makes sure deletion is only performed in the compilation unit of
    /// the plugin
    template <typename T>
    struct custom_deleter
    {
        void operator() (T *pObj)
        { pObj->Destroy(); }
    };

public:

    // automatic cloning: do NOT override unless you know what you're doing!
    virtual boost::shared_ptr<ResolverService> clone()
    { return boost::shared_ptr<TPluginType>( new TPluginType(), custom_deleter<TPluginType>() ); };

};

////////////////////////////////////////////////////////////////////////////////

}

#endif // #ifndef __PLAYDAR_PLUGIN_INCLUDE_H

