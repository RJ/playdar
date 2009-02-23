#include "resolvers/playable_item.h"
#include <cassert>
using namespace std;

// get JSON representation
json_spirit::Object 
PlayableItem::get_json() const
{
    using namespace json_spirit;
    Object j;
    j.push_back( Pair("sid", id()) );
    j.push_back( Pair("artist", artist()) );
    j.push_back( Pair("album",  album()) );
    j.push_back( Pair("track", track()) );
    j.push_back( Pair("source", source()) );
    j.push_back( Pair("size", size()) );
    j.push_back( Pair("mimetype", mimetype()) );
    j.push_back( Pair("bitrate", bitrate()) );
    j.push_back( Pair("duration", duration()) );
    //j.push_back( Pair("url", sid_to_url(id())) );
    j.push_back( Pair("score", (double)score()) );
    j.push_back( Pair("preference", (double)preference()) );
    return j;
}

boost::shared_ptr<PlayableItem> 
PlayableItem::from_json(json_spirit::Object resobj)
{
    string artist, album, track, sid, source, mimetype;
    int size, bitrate, duration = 0;
    float score = 0;
    
    using namespace json_spirit;
    map<string,Value> resobj_map;
    obj_to_map(resobj, resobj_map);
        
    if(resobj_map.find("artist")!=resobj_map.end())     artist  = resobj_map["artist"].get_str();
    if(resobj_map.find("album")!=resobj_map.end())      album   = resobj_map["album"].get_str();
    if(resobj_map.find("track")!=resobj_map.end())      track   = resobj_map["track"].get_str();
    if(resobj_map.find("sid")!=resobj_map.end())        sid     = resobj_map["sid"].get_str();
    if(resobj_map.find("source")!=resobj_map.end())     source  = resobj_map["source"].get_str();
    if(resobj_map.find("mimetype")!=resobj_map.end())   mimetype= resobj_map["mimetype"].get_str();
    
    if(resobj_map.find("size")!=resobj_map.end())       size    = resobj_map["size"].get_int();
    if(resobj_map.find("bitrate")!=resobj_map.end())    bitrate = resobj_map["bitrate"].get_int();
    if(resobj_map.find("duration")!=resobj_map.end())   duration= resobj_map["duration"].get_int();
    
    if(resobj_map.find("score")!=resobj_map.end())      score   = (float)resobj_map["score"].get_real();
    
    if(!artist.length() && !track.length()) throw;
    
    boost::shared_ptr<PlayableItem> pip( new PlayableItem(artist, album, track) );
    
    if(sid.length())        pip->set_id(sid);
    if(source.length())     pip->set_source(source);
    if(mimetype.length())   pip->set_mimetype(mimetype);
    if(size)                pip->set_size(size);
    if(bitrate)             pip->set_bitrate(bitrate);
    if(duration)            pip->set_duration(duration);
    if(score)               pip->set_score(score);
                    
    return pip;
}

const source_uid & 
PlayableItem::id() const   
{ 
    if(m_uuid.length()==0) // generate it if not already specified
    {
        m_uuid = MyApplication::gen_uuid();
    }
    return m_uuid; 
}


