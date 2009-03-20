#ifndef __PLAYDAR_AMPACHE_PLUGIN_H__
#define __PLAYDAR_AMPACHE_PLUGIN_H__

#include "playdar/playdar_plugin_include.h"
 
namespace playdar
{
    namespace resolvers
    {
        class ampache : public ResolverService
        {
            string auth;
            
            struct Query
            {
                boost::shared_ptr<ResolverQuery> rq;
                string filter() const { return rq->artist() + ' ' + rq->track(); }
                int id() const { return rq->id(); }
            };
            
            std::map<int, Query> queries;
            
        public:
            virtual void init(playdar::Config * c, Resolver * r);
 
            void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    
            std::string name() const { return "Ampache"; }
    
        protected:
            ~demo() throw() {};
         };
 
        // This makes the plugin loading possible:
        EXPORT_DYNAMIC_CLASS( ampache )
 
    } // resolvers
} // playdar
#endif
