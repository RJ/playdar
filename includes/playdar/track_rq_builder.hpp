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
#ifndef __TRACK_RESOLVER_QUERY__
#define __TRACK_RESOLVER_QUERY__

#include "resolver_query.hpp"

namespace playdar {

namespace TrackRQBuilder {
    static rq_ptr build( const std::string& artist, const std::string& album, const std::string& track )
    {
        rq_ptr rq = rq_ptr( new ResolverQuery );
        rq->set_param( "artist", artist );
        rq->set_param( "album", album );
        rq->set_param( "track", track );
        return rq;
    }

} //namespace TrackRQBuilder

} //namespace playdar

#endif //__TRACK_RESOLVER_QUERY__
