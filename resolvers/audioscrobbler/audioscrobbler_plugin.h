/** @author Max Howell <max@methylblue.com> 
  * @brief Scrobbling support for playdar clients 
  */

#include "playdar/playdar_plugin_include.h"

namespace playdar
{
    namespace plugin //TODO not a resolver
    {
        class audioscrobbler : public ResolverPlugin<audioscrobbler>
        {
            bool auth_required;
            
            static void scrobsub_callback(int, const char*);
            
        public:
            audioscrobbler() : auth_required(false)
            {}
            
            virtual bool init( pa_ptr );
            virtual void Destroy();
            virtual std::string name() const { return "Audioscrobbler"; }
            virtual playdar_response http_handler(const playdar_request&, playdar::auth* pauth);

        private:
            /** pure virtual, so reimplemented but does nothing */
            void start_resolving(boost::shared_ptr<ResolverQuery> rq)
            {}
        };
    }
}
