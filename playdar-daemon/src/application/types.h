#ifndef __MYAPPLICATION_TYPES_H__
#define __MYAPPLICATION_TYPES_H__
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <stdio.h>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

using namespace std;

typedef string query_uid;  // identifies a resolverquery
typedef string source_uid; // identifies a streamable source for a song

class Artist;
class Album;
class Track;
typedef boost::shared_ptr<Artist>   artist_ptr;
typedef boost::shared_ptr<Track>    track_ptr;
typedef boost::shared_ptr<Album>    album_ptr;

// Callback type for observing new RQ results:
class PlayableItem; // fwd
typedef boost::function< void (query_uid qid, boost::shared_ptr<PlayableItem> pip)> rq_callback_t;

struct scorepair
{
    int id;
    float score;
};

// sort by anything that has a score field, descending order
struct sortbyscore
{
    template<typename T>
    bool operator () (const T & lhs, const T & rhs){
        return lhs.score > rhs.score;
    }
};

#endif

