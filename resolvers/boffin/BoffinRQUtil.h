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
#include "playdar/playdar_plugin_include.h"
#include "playdar/resolver_query.hpp"
#include "playdar/types.h"
#include <string>

namespace BoffinRQUtil
{
    playdar::rq_ptr buildTagCloudRequest( const std::string& rql )
    {
        playdar::rq_ptr rq( new playdar::ResolverQuery );
        rq->set_param( "boffin_tags", rql );
        return rq;
    }
    
    playdar::rq_ptr buildRQLRequest( const std::string& rql )
    {
        playdar::rq_ptr rq( new playdar::ResolverQuery );
        rq->set_param( "boffin_rql", rql );
        return rq;        
    }
}