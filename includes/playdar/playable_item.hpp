#ifndef __PLAYABLE_ITEM_H__
#define __PLAYABLE_ITEM_H__
#include "playdar/resolved_item.h"
#include "playdar/config.hpp"
#include "playdar/types.h"
#include "playdar/ss_http.hpp"
#include "json_spirit/json_spirit.h"
#include <cassert>
/*
    Represents something (a song) that can be played.
    ResolverService plugins provide these as their results for matching content.
    
    The attached StreamingStrategy embodies how to access the song - this PlayableItem
    could refer to a local file, or a remote file. Clients use the "sid" to access it.
*/
using namespace std;

class PlayableItem : public ResolvedItem
{
public:
    PlayableItem()
    {
        set_duration(0);
        set_tracknum(0);
        set_size(0);
        set_bitrate(0);
        set_mimetype("text/plain");
        set_source("unspecified");
    }
    
    PlayableItem(string art, string alb, string trk)
    :m_score( -1.0f )
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
    
    ~PlayableItem()
    {
        cout << "dtor, playableitem: " << id() << endl;
    }
    
    static bool is_valid_json( const json_spirit::Object& obj )
    {
        map<string, json_spirit::Value> objMap;
        obj_to_map( obj, objMap );
        if( objMap.find( "artist" ) == objMap.end())
            return false;
        if( objMap.find( "track" ) == objMap.end())
            return false;
        if( objMap.find( "url" ) == objMap.end())
            return false;
        
        return true;
    }
    
    static boost::shared_ptr<PlayableItem> from_json( const json_spirit::Object& resobj )
    {
        string artist, album, track, sid, source, mimetype, url;
        int size = 0, bitrate = 0, duration = 0;
        float score = -1;
        
        using namespace json_spirit;
        map<string,Value> resobj_map;
        obj_to_map(resobj, resobj_map);
            
        if(resobj_map.find("artist")!=resobj_map.end())
            artist  = resobj_map["artist"].get_str();
            
        if(resobj_map.find("album")!=resobj_map.end())
            album   = resobj_map["album"].get_str();
            
        if(resobj_map.find("track")!=resobj_map.end())
            track   = resobj_map["track"].get_str();
            
        if(resobj_map.find("sid")!=resobj_map.end())
            sid     = resobj_map["sid"].get_str();
            
        if(resobj_map.find("url")!=resobj_map.end())
            url     = resobj_map["url"].get_str();
            
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
        if(url.length())        pip->set_url(url);
                        
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
        j.push_back( Pair("url", url())             );
    }
    
    void set_artist(string s)   { m_artist = s; }
    void set_album(string s)    { m_album  = s; }
    void set_track(string s)    { m_track  = s; }
    void set_url(string s)      { m_url    = s; }
    void set_mimetype(string s) { m_mimetype = s; }
    void set_duration(int s)    { m_duration = s; }
    void set_tracknum(int s)    { m_tracknum = s; }
    void set_size(int s)        { m_size = s; }
    void set_bitrate(int s)     { m_bitrate = s; }
    void set_streaming_strategy(boost::shared_ptr<StreamingStrategy> s)   { m_ss = s; }
    
    const string & artist() const   { return m_artist; }
    const string & album() const    { return m_album; }
    const string & track() const    { return m_track; }
    const string & mimetype() const { return m_mimetype; }
    const string & url() const      { return m_url; }
    const int duration() const      { return m_duration; }
    const int bitrate() const       { return m_bitrate; }
    const int tracknum() const      { return m_tracknum; }
    const int size() const          { return m_size; }
    boost::shared_ptr<StreamingStrategy> streaming_strategy() const 
    {
        // memoized auto-upgrade from an url param -> httpstreamingstrategy:
        if(m_ss) return m_ss; 
        if(!m_ss && m_url.length())
        {
            m_ss = boost::shared_ptr<StreamingStrategy>
            (new HTTPStreamingStrategy(m_url));
        }
        return m_ss; // could be null if not specified.
    }
    
private:
    string m_artist;
    string m_album;
    string m_track;
    string m_mimetype;
    string m_url;
    int m_size;
    int m_duration;
    int m_bitrate;
    int m_tracknum;
    float m_score;
    string m_source;
    
    mutable boost::shared_ptr<StreamingStrategy> m_ss;
};
#endif

