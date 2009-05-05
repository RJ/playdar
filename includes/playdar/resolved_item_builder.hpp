#ifndef __RESOLVED_ITEM_BUILDER_H__
#define __RESOLVED_ITEM_BUILDER_H__
#include "playdar/resolved_item.h"
#include "playdar/library.h"

namespace playdar {

/*
    Builds a ResolvedItem describing something (a song) that can be played.
*/

class ResolvedItemBuilder
{
public:

    static ri_ptr createFromFid(Library& lib, int fid)
    {
        return createFromFid( lib.db(), fid );
    }
    
    static ri_ptr createFromFid( sqlite3pp::database& db, int fid )
    {
        LibraryFile_ptr file( Library::file_from_fid(db, fid) );
        
        ri_ptr rip( new ResolvedItem() );
        rip->set_json_value( "mimetype", file->mimetype );
        rip->set_json_value( "size", file->size );
        rip->set_json_value( "duration", file->duration );
        rip->set_json_value( "bitrate", file->bitrate );
        artist_ptr artobj = Library::load_artist( db, file->piartid);
        track_ptr trkobj = Library::load_track( db, file->pitrkid);
        rip->set_json_value( "artist", artobj->name());
        rip->set_json_value( "track", trkobj->name());
        // album metadata kinda optional for now
        if (file->pialbid) {
            album_ptr albobj = Library::load_album(db, file->pialbid);
            rip->set_json_value( "album", albobj->name());
        }
        rip->set_json_value( "url", file->url );
        return rip;
    }
    
};

} // ns

#endif //__RESOLVED_ITEM_BUILDER_H__
