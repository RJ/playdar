/** @author <max@methylblue.com> 
  * @brief Scrobbling support for playdar clients 
  */

#ifndef __PLAYDAR_AUDIOSCROBBLER_PLUGIN_H__
#define __PLAYDAR_AUDIOSCROBBLER_PLUGIN_H__

#include "playdar/playdar_plugin_include.h"

namespace playdar
{
    namespace resolvers //TODO not a resolver
    {
        class audioscrobbler : public ResolverServicePlugin
        {
        public:
            virtual bool init(playdar::Config*, Resolver*);
            virtual void Destroy();
            virtual string name() const { return "Audioscrobbler"; }
            virtual string http_handler(const playdar_request&, playdar::auth* pauth);

        private:
            /** pure virtual, so reimplemented but does nothing */
            void start_resolving(boost::shared_ptr<ResolverQuery> rq)
            {}
        };

        EXPORT_DYNAMIC_CLASS( audioscrobbler )
    }
}
#endif
