/*
    Playdar - music content resolver
    Copyright (C) 2009  Richard Jones
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

