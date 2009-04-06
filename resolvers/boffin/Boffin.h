#ifndef __RS_BOFFIN_H__
#define __RS_BOFFIN_H__

#include "playdar/playdar_plugin_include.h"


class Boffin : public ResolverService
{
public:
    Boffin();
   
    virtual std::string name() const;
    
    /// max time in milliseconds we'd expect to have results in.
    virtual unsigned int target_time() const;
    
    /// highest weighted resolverservices are queried first.
    virtual unsigned short weight() const;

    virtual void start_resolving(boost::shared_ptr<ResolverQuery> rq);

    /** thread-safe */
    virtual bool report_results(query_uid qid, 
        vector< boost::shared_ptr<PlayableItem> > results,
        string via);
    
    // default is empty, ie no http urls handle
    virtual vector<string> get_http_handlers();

    // handler for HTTP reqs we are registerd for:
    virtual string http_handler( string url,
                                 vector<string> parts,
                                 map<string,string> getvars,
                                 map<string,string> postvars,
                                playdar::auth * pauth);

};

#endif