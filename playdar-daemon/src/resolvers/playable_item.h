#ifndef __PLAYABLE_ITEM_H__
#define __PLAYABLE_ITEM_H__
#include "application/application.h"
#include "resolvers/streaming_strategy.h"
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
    }
    
    PlayableItem(string art, string alb, string trk)
    {
        set_artist(art);
        set_album(alb);
        set_track(trk);
        set_duration(0);
        set_tracknum(0);
        set_score(0);
        set_size(0);
        set_bitrate(0);
        set_mimetype("text/plain");
        set_preference(1.0);
        set_source("unspecified");
        //set_url("NO_URL");
    }
    
    ~PlayableItem()
    {
        cout << "dtor, playableitem" << endl;
    }
    
    static boost::shared_ptr<PlayableItem> from_json(json_spirit::Object resobj);
    json_spirit::Object get_json() const;

    const source_uid & id() const;
    // setters
    void set_streaming_strategy(boost::shared_ptr<StreamingStrategy> s)   { m_ss = s; }
    
    void set_artist(string s)   { m_artist = s; }
    void set_album(string s)    { m_album  = s; }
    void set_track(string s)    { m_track  = s; }
    //void set_url(string s)      { m_url    = s; }
    void set_source(string s)   { m_source = s; }
    void set_mimetype(string s)   { m_mimetype = s; }
    void set_duration(int s)    { m_duration = s; }
    void set_tracknum(int s)    { m_tracknum = s; }
    void set_size(int s)    { m_size = s; }
    void set_bitrate(int s)    { m_bitrate = s; }
    void set_score(float s)     
    { 
        assert(s <= 1.0);
        assert(s >= 0);
        m_score  = s; 
    }
    void set_preference(float s)   // bigger = faster network
    { 
        assert(s <= 1.0);
        assert(s >= 0);
        m_preference = s; 
    } 
    
    // getters
    boost::shared_ptr<StreamingStrategy> streaming_strategy() const { return m_ss; }
    
    void set_id(string s) { m_uuid = s; }

    
    const string & artist() const   { return m_artist; }
    const string & album() const    { return m_album; }
    const string & track() const    { return m_track; }
    const string & source() const   { return m_source; }
    const string & mimetype() const { return m_mimetype; }
    const float score() const       { return m_score; }
    const int duration() const      { return m_duration; }
    const int bitrate() const       { return m_bitrate; }
    const int tracknum() const      { return m_tracknum; }
    const int size() const          { return m_size; }
    const float preference() const  { return 1.0 ; }
    // this doesn't bode well..
    //const float preference() const  { assert(m_preference >= 0); assert(m_preference <= 1); return m_preference; }

    
    
private:
    boost::shared_ptr<StreamingStrategy> m_ss;
    mutable source_uid m_uuid;
    string m_artist;
    string m_album;
    string m_track;
    string m_mimetype;
    int m_size;
    int m_duration;
    int m_bitrate;
    int m_tracknum;
    float m_preference;
    float m_score;
    string m_source;
};
#endif

