#ifndef __RS_BOFFIN_H__
#define __RS_BOFFIN_H__

#include "playdar/playdar_plugin_include.h"
#include <queue>
#include <boost/function.hpp>
#include <boost/thread.hpp>


class Boffin : public ResolverServicePlugin
{
public:
    Boffin();

    // return false to disable the plugin
    virtual bool init(playdar::Config*, Resolver*);
   
    virtual std::string name() const;
    
    /// max time in milliseconds we'd expect to have results in.
    virtual unsigned int target_time() const;
    
    /// highest weighted resolverservices are queried first.
    virtual unsigned short weight() const;

    virtual void start_resolving(boost::shared_ptr<ResolverQuery> rq);

    // default is empty, ie no http urls handle
    virtual vector<string> get_http_handlers();

    // handler for HTTP reqs we are registerd for:
    virtual string http_handler( string url,
                                 vector<string> parts,
                                 map<string,string> getvars,
                                 map<string,string> postvars,
                                playdar::auth * pauth);

private:
    void resolve(boost::shared_ptr<ResolverQuery> rq);

    void thread_run();
    void queue_work(boost::function< void() > work);
    boost::function< void() > get_work();
    void drain_queue();
    void stop();

    volatile bool m_thread_stop;
    boost::thread* m_thread;
    boost::mutex m_queue_mutex;
    boost::condition_variable m_queue_wake;
    std::queue< boost::function< void() > > m_queue;
};

EXPORT_DYNAMIC_CLASS( Boffin )


#endif

