#include "Boffin.h"


Boffin::Boffin()
{
}

std::string 
Boffin::name() const
{
    return "Boffin";
}

/// max time in milliseconds we'd expect to have results in.
unsigned int 
Boffin::target_time() const
{
    return 50;
}

/// highest weighted resolverservices are queried first.
unsigned short 
Boffin::weight() const
{
    return 1000;
}

void 
Boffin::start_resolving(boost::shared_ptr<ResolverQuery> rq)
{
}

/** thread-safe */
bool 
Boffin::report_results(query_uid qid, 
                       vector< boost::shared_ptr<PlayableItem> > results, 
                       string via)
{
    return false;
}

// default is empty, ie no http urls handle
vector<string> 
Boffin::get_http_handlers()
{
    vector<string> h;
    h.push_back("boffin");
    return h;
}

// handler for HTTP reqs we are registerd for:
string 
Boffin::http_handler(string url,
                     vector<string> parts,
                     map<string,string> getvars,
                     map<string,string> postvars,
                     playdar::auth * pauth)
{
    return "This plugin has no web interface.";
}
