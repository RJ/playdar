#ifndef __MYAPPLICATION_TYPES_H__
#define __MYAPPLICATION_TYPES_H__

#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

typedef std::string query_uid;  // identifies a resolverquery
typedef std::string source_uid; // identifies a streamable source for a song

class Artist;
class Album;
class Track;
class LibraryFile;
typedef boost::shared_ptr<Artist>       artist_ptr;
typedef boost::shared_ptr<Track>        track_ptr;
typedef boost::shared_ptr<Album>        album_ptr;
typedef boost::shared_ptr<LibraryFile>  LibraryFile_ptr;

class ResolvedItem;
class PlayableItem;
class ResolverQuery;
class PluginAdaptor;
class StreamingStrategy;
typedef boost::shared_ptr<PlayableItem>     pi_ptr;
typedef boost::shared_ptr<ResolvedItem>     ri_ptr;
typedef boost::shared_ptr<ResolverQuery>    rq_ptr;
typedef boost::shared_ptr<PluginAdaptor>    pa_ptr;
typedef boost::shared_ptr<StreamingStrategy> ss_ptr;




// Callback type for observing new RQ results:
class PlayableItem; // fwd
typedef boost::function< void (query_uid qid, ri_ptr pip)> rq_callback_t;

/// Handlers for web requests:
typedef boost::function< void  (const std::string url,
                                const std::vector<std::string> parts,
                                const std::map<std::string, std::string> getvars,
                                const std::map<std::string, std::string> postvars) > 
         http_req_cb;

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

