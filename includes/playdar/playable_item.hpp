#ifndef __PLAYABLE_ITEM_H__
#define __PLAYABLE_ITEM_H__
#include "playdar/resolved_item.h"
#include "playdar/config.h"
#include "playdar/library.h"
#include "playdar/ss_curl.hpp"
#include "json_spirit/json_spirit.h"

#include <cassert>
#include <string>
#include <map>

/*
    Represents something (a song) that can be played.
    ResolverService plugins provide these as their results for matching content.
    
    The attached StreamingStrategy embodies how to access the song - this PlayableItem
    could refer to a local file, or a remote file. Clients use the "sid" to access it.
*/

class PlayableItem : public ResolvedItem
{
public:
    PlayableItem()
    : m_score( -1.0f )
    {
        set_duration(0);
        set_tracknum(0);
        set_size(0);
        set_bitrate(0);
        set_mimetype("text/plain");
        set_source("unspecified");
    }
    
    PlayableItem(std::string art, std::string alb, std::string trk)
    : m_score( -1.0f )
    {
        set_artist(art);
        set_album(alb);
        set_track(trk);
        
        set_duration(0);
        set_tracknum(0);
        set_size(0);
        set_bitrate(0);
        set_mimetype("text/plain");
        set_source("unspecified");
    }

    static
    pi_ptr 
    create(Library& lib, int fid)
    {
        LibraryFile_ptr file( lib.file_from_fid(fid) );
        pi_ptr pip( new PlayableItem() );
        pip->set_mimetype( file->mimetype );
        pip->set_size( file->size );
        pip->set_duration( file->duration );
        pip->set_bitrate( file->bitrate );
        //    int m_tracknum;
        //    float m_score;
        //    string m_source;
        artist_ptr artobj = lib.load_artist(file->piartid);
        track_ptr trkobj = lib.load_track(file->pitrkid);
        pip->set_artist(artobj->name());
        pip->set_track(trkobj->name());
        // album metadata kinda optional for now
        if (file->pialbid) {
            album_ptr albobj = lib.load_album(file->pialbid);
            pip->set_album(albobj->name());
        }
        boost::shared_ptr<StreamingStrategy> ss(new CurlStreamingStrategy(file->url));
        pip->set_streaming_strategy(ss);
        return pip;
    }

    ~PlayableItem()
    {
        std::cout << "dtor, playableitem: " << id() << std::endl;
    }
    
    static bool is_valid_json( const json_spirit::Object& obj )
    {
        std::map<std::string, json_spirit::Value> objMap;
        obj_to_map( obj, objMap );
        if( objMap.find( "artist" ) == objMap.end())
            return false;
        if( objMap.find( "track" ) == objMap.end())
            return false;
        
        return true;
    }
    
    static boost::shared_ptr<PlayableItem> from_json( const json_spirit::Object& resobj )
    {
        std::string artist, album, track, sid, source, mimetype, url;
        int size = 0, bitrate = 0, duration = 0;
        float score = -1;
        
        using namespace json_spirit;
        std::map<std::string,Value> resobj_map;
        obj_to_map(resobj, resobj_map);
            
        if(resobj_map.find("artist")!=resobj_map.end())
            artist  = resobj_map["artist"].get_str();
            
        if(resobj_map.find("album")!=resobj_map.end())
            album   = resobj_map["album"].get_str();
            
        if(resobj_map.find("track")!=resobj_map.end())
            track   = resobj_map["track"].get_str();
            
        if(resobj_map.find("sid")!=resobj_map.end())
            sid     = resobj_map["sid"].get_str();
            
        if(resobj_map.find("source")!=resobj_map.end())
            source  = resobj_map["source"].get_str();
            
        if(resobj_map.find("mimetype")!=resobj_map.end())
            mimetype= resobj_map["mimetype"].get_str();
            
        
        if(resobj_map.find("size")!=resobj_map.end())
            size    = resobj_map["size"].get_int();
            
        if(resobj_map.find("bitrate")!=resobj_map.end())
            bitrate = resobj_map["bitrate"].get_int();
            
        if(resobj_map.find("duration")!=resobj_map.end())
            duration= resobj_map["duration"].get_int();
        
        if(resobj_map.find("score")!=resobj_map.end())
        {
            if(resobj_map["score"].type() == int_type)
                score = (float) (resobj_map["score"].get_int());
            else 
            if (resobj_map["score"].type() == real_type)
                score = (float) (resobj_map["score"].get_real());
            else
                score = (float) -1;
        }
        
        if(!artist.length() && !track.length()) throw;
        
        boost::shared_ptr<PlayableItem> 
            pip( new PlayableItem(artist, album, track) );
        
        if(sid.length())        pip->set_id(sid);
        if(source.length())     pip->set_source(source);
        if(mimetype.length())   pip->set_mimetype(mimetype);
        if(size)                pip->set_size(size);
        if(bitrate)             pip->set_bitrate(bitrate);
        if(duration)            pip->set_duration(duration);
        if(score >= 0)          pip->set_score(score);
        
        return pip;
    }
    
    void create_json( json_spirit::Object& j ) const
    {
        using namespace json_spirit;
        j.push_back( Pair("sid", id())              );
        j.push_back( Pair("artist", artist())       );
        j.push_back( Pair("album",  album())        );
        j.push_back( Pair("track", track())         );
        j.push_back( Pair("source", source())       );
        j.push_back( Pair("size", size())           );
        j.push_back( Pair("mimetype", mimetype())   );
        j.push_back( Pair("bitrate", bitrate())     );
        j.push_back( Pair("duration", duration())   );
    }
    
    void set_artist(std::string s)   { m_artist = s; }
    void set_album(std::string s)    { m_album  = s; }
    void set_track(std::string s)    { m_track  = s; }

    void set_mimetype(std::string s) { m_mimetype = s; }
    void set_duration(int s)         { m_duration = s; }
    void set_tracknum(int s)         { m_tracknum = s; }
    void set_size(int s)             { m_size = s; }
    void set_bitrate(int s)          { m_bitrate = s; }

    void set_streaming_strategy(boost::shared_ptr<StreamingStrategy> s)   { m_ss = s; }
    
    const std::string & artist() const   { return m_artist; }
    const std::string & album() const    { return m_album; }
    const std::string & track() const    { return m_track; }
    const std::string & mimetype() const { return m_mimetype; }

    const int duration() const           { return m_duration; }
    const int bitrate() const            { return m_bitrate; }
    const int tracknum() const           { return m_tracknum; }
    const int size() const               { return m_size; }
    /// This returns what ss->get_instance() returns, which in some cases is a 
    /// shared_ptr to the same SS as is attached, if the SS is threadsafe.
    /// in the case of curlSS, it's a copy, because curlSS is not threadsafe.
    boost::shared_ptr<StreamingStrategy> streaming_strategy() const 
    {
        if(m_ss) return m_ss->get_instance();
        // this returns a null shared_ptr:
        return m_ss;
    }
    
private:
    std::string m_artist;
    std::string m_album;
    std::string m_track;
    std::string m_mimetype;
    int m_size;
    int m_duration;
    int m_bitrate;
    int m_tracknum;
    float m_score;
    std::string m_source;
    
    boost::shared_ptr<StreamingStrategy> m_ss;
};

#endif
