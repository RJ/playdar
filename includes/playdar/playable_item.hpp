#ifndef __PLAYABLE_ITEM_H__
#define __PLAYABLE_ITEM_H__
#include "playdar/config.hpp"
#include "playdar/streaming_strategy.h"
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

class PlayableItem
{
public:
    PlayableItem()
    {
        set_duration(0);
        set_tracknum(0);
        set_score(0);
        set_size(0);
        set_bitrate(0);
        set_mimetype("text/plain");
        set_source("unspecified");
    }
    
    PlayableItem(string art, string alb, string trk)
    {
        PlayableItem();
        set_artist(art);
        set_album(alb);
        set_track(trk);
    }
    
    ~PlayableItem()
    {
        //cout << "dtor, playableitem" << endl;
    }
    
    static boost::shared_ptr<PlayableItem> from_json(json_spirit::Object resobj)
    {
        string artist, album, track, sid, source, mimetype, url;
        int size, bitrate, duration = 0;
        float score = 0;
        
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
                score = (float) 0.0;
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
        if(score)               pip->set_score(score);
        if(url.length())        pip->set_url(url);
                        
        return pip;
    }
    
    json_spirit::Object get_json() const
    {
        using namespace json_spirit;
        Object j;
        j.push_back( Pair("_msgtype", "pi")         );
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
        j.push_back( Pair("score", (double)score()) );
        return j;
    }

    const source_uid & id() const
    {
        if(m_uuid.length()==0) // generate it if not already specified
        {
            m_uuid = playdar::Config::gen_uuid();
        }
        return m_uuid; 
    }
    
    // setters
    void set_streaming_strategy(boost::shared_ptr<StreamingStrategy> s)   { m_ss = s; }
    
    void set_artist(string s)   { m_artist = s; }
    void set_album(string s)    { m_album  = s; }
    void set_track(string s)    { m_track  = s; }
    void set_url(string s)      { m_url    = s; }
    void set_source(string s)   { m_source = s; }
    void set_mimetype(string s) { m_mimetype = s; }
    void set_duration(int s)    { m_duration = s; }
    void set_tracknum(int s)    { m_tracknum = s; }
    void set_size(int s)        { m_size = s; }
    void set_bitrate(int s)     { m_bitrate = s; }
    void set_score(float s)
    { 
        assert(s <= 1.0);
        assert(s >= 0);
        m_score  = s; 
    }
    
    // getters
    boost::shared_ptr<StreamingStrategy> streaming_strategy() const 
    {
        // TODO (make this mutable and call set_ss on first call?) 
        if(m_ss) return m_ss; 
        if(!m_ss && m_url.length())
        {
            return boost::shared_ptr<StreamingStrategy>
                    (new HTTPStreamingStrategy(m_url));
        }
        return m_ss; // would be null if not given.
    }
    
    void set_id(string s) { m_uuid = s; }

    const string & artist() const   { return m_artist; }
    const string & album() const    { return m_album; }
    const string & track() const    { return m_track; }
    const string & source() const   { return m_source; }
    const string & mimetype() const { return m_mimetype; }
    const string & url() const      { return m_url; }
    const float score() const       { return m_score; }
    const int duration() const      { return m_duration; }
    const int bitrate() const       { return m_bitrate; }
    const int tracknum() const      { return m_tracknum; }
    const int size() const          { return m_size; }
    
private:
    boost::shared_ptr<StreamingStrategy> m_ss;
    mutable source_uid m_uuid;
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
};
#endif

