#include "ampache_plugin.h"
#include <boost/algorithm/string.hpp>

namespace playdar {
namespace resolvers {

void
ampache::init(playdar::Config * c, Resolver * r)
{
    ResolverService::init( c, r );
    handshake();
}

void
ampache::handshake()
{
    auth = "";

    string time = time();
    string passphrase = md5(time + password());
    get("action=handshake&auth=" + passphrase + "&timestamp=" + time + "&user=mxcl");
}

void
start_resolving(boost::shared_ptr<ResolverQuery> rq)
{
    const int id = rq->id();
    Query& q = queries[id];
    q.rq = rq;

    if(auth.empty())
        handshake();
    else
        get(id, "action=searchsongs&auth=" + auth + "&filter=" + q.filter());
}

void
got(int id, const string& data)
{
    boost::xml xml(data);
    //blah
    
    switch(error){
        case 400:
        case 403:
        case 405:
        case 501:
            log( xml );
            return;
        case 401:
            handshake();
            return;
    }
    
    boost::shared_ptr<PlayableItem> pip(new PlayableItem(artist, album, track));                      
    pip->set_score(1.0);    // Exact match, so set maximum score
    pip->set_bitrate(128);  // avg. bitrate in kbps
    pip->set_size(3401187); // filesize
    pip->set_duration(203); // play duration, seconds

    boost::shared_ptr<StreamingStrategy> strategy(new HTTPStreamingStrategy(url));
    // Attach streamingstrat to the playable item:
    pip->set_streaming_strategy(strategy);
    // Results are a list of playableitems:
    vector<boost::shared_ptr<PlayableItem> > v;
    // .. containing just the one match we found:
    v.push_back(pip);
    
    report_results(queries[id], v, name());
}

}} //namespaces
