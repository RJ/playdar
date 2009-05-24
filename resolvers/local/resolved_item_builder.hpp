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
#ifndef __RESOLVED_ITEM_BUILDER_H__
#define __RESOLVED_ITEM_BUILDER_H__
#include "playdar/resolved_item.h"
#include "library.h"

using namespace json_spirit;

namespace playdar {

/*
    Builds a ResolvedItem describing something (a song) that can be played.
*/

class ResolvedItemBuilder
{
public:

    static void createFromFid(Library& lib, int fid, json_spirit::Object& out)
    {
        createFromFid( lib.db(), fid, out );
    }
    
    static void createFromFid( sqlite3pp::database& db, int fid, Object& out)
    {
        LibraryFile_ptr file( Library::file_from_fid(db, fid) );
        
        out.push_back( Pair("mimetype", file->mimetype) );
        out.push_back( Pair("size", file->size) );
        out.push_back( Pair("duration", file->duration) );
        out.push_back( Pair("bitrate", file->bitrate) );
        artist_ptr artobj = Library::load_artist( db, file->piartid);
        track_ptr trkobj = Library::load_track( db, file->pitrkid);
        out.push_back( Pair("artist", artobj->name()) );
        out.push_back( Pair("track", trkobj->name()) );
        // album metadata kinda optional for now
        if (file->pialbid) {
            album_ptr albobj = Library::load_album(db, file->pialbid);
            out.push_back( Pair("album", albobj->name()) );
        }
        out.push_back( Pair("url", file->url) );
    }
    
};

} // ns

#endif //__RESOLVED_ITEM_BUILDER_H__
