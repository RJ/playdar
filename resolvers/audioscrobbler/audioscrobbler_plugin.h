/*
    Playdar - music content resolver
    Copyright (C) 2009  Last.fm Ltd.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
            
            virtual bool init(pa_ptr);
            virtual void Destroy();
            virtual std::string name() const { return "Audioscrobbler"; }
            virtual bool authed_http_handler(const playdar_request&, playdar_response&, playdar::auth* pauth);

        private:
            /** pure virtual, so reimplemented but does nothing */
            void start_resolving(boost::shared_ptr<ResolverQuery> rq)
            {}
        };
    }
}
