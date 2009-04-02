#include "demo.h"
#include <boost/algorithm/string.hpp>

namespace playdar {
namespace resolvers {

/*
    Called once when plugin is loaded when playdar starts up.
*/
bool
demo::init(playdar::Config * c, Resolver * r)
{
    m_resolver  = r;
    m_conf = c;
    cout << "Demo resolver init(): "
         << "this only finds 'Sweet Melissa' by the artist 'Big Bad Sun'"
         << endl;
    return true;
}

/*
    The start_resolving call must not block. Most plugins will have their own
    threading model, and would typically do any slow searching in a new thread.
    
    This example does not use threads, because it just matches a single
    hardcoded track, and is thus very fast.
    
    A ResolverQuery is passed in - this represents the search to execute.
    
    Notice this call returns void - the resolving framework is asynchronous,
    and you can call report_results() at any time after the search was started,
    when a result is found.
*/
void
demo::start_resolving(boost::shared_ptr<ResolverQuery> rq)
{
    // we'll only resolve an exact match to one song.
    if( boost::to_lower_copy(rq->artist())  == "big bad sun" &&
        boost::to_lower_copy(rq->track())   == "sweet melissa" )
    {
        // PlayableItems represent results - they hold metadata
        // for the track we resolved the search to, and extra data
        // such as the score (how good a match was it?)
        // the bitrate, filesize, etc.
        boost::shared_ptr<PlayableItem> pip
            (new PlayableItem("Big Bad Sun", 
                              "", // no album name specified
                              "Sweet Melissa"));
                              
        pip->set_score(1.0);    // Exact match, so set maximum score
        pip->set_bitrate(128);  // avg. bitrate in kbps
        pip->set_size(3401187); // filesize
        pip->set_duration(203); // play duration, seconds
        
        // Create streaming strategy for accessing the track:
        string url = "http://he3.magnatune.com/all/01-Sweet%20Melissa-Big%20Bad%20Sun.mp3";
        boost::shared_ptr<StreamingStrategy> 
                    s(new HTTPStreamingStrategy(url));
        // Attach streamingstrat to the playable item:
        pip->set_streaming_strategy(s);
        // Results are a list of playableitems:
        vector< boost::shared_ptr<PlayableItem> > v;
        // .. containing just the one match we found:
        v.push_back(pip);
        // tell the resolver what we found:
        report_results(rq->id(), v, name());
    }
    else
    {
        // just do nothing.
        // resolvers should only respond if they find a match.
    }
}


} // resolvers
} // playdar
