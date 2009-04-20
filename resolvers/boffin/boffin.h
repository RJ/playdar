#ifndef __RS_BOFFIN_H__
#define __RS_BOFFIN_H__

#include "playdar/playdar_plugin_include.h"
#include <queue>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

class boffin : public playdar::ResolverPlugin<boffin>
{
public:
    boffin();

    // return false to disable the plugin
    virtual bool init(playdar::pa_ptr pap);
   
    virtual std::string name() const;
    
    /// max time in milliseconds we'd expect to have results in.
    virtual unsigned int target_time() const;
    
    /// highest weighted resolverservices are queried first.
    virtual unsigned short weight() const;

    virtual void start_resolving(boost::shared_ptr<playdar::ResolverQuery> rq);

    // default is empty, ie no http urls handle
    virtual std::vector<std::string> get_http_handlers();

    // handler for HTTP reqs we are registerd for:
    virtual playdar::playdar_response http_handler( const playdar::playdar_request*, playdar::auth * pauth);

protected:
    virtual ~boffin() throw();

private:
    void resolve(boost::shared_ptr<playdar::ResolverQuery> rq);
    void parseFail(std::string line, int error_offset);

    /// thread work queue stuff [

    void thread_run();
    void queue_work(boost::function< void() > work);
    boost::function< void() > get_work();

    volatile bool m_thread_stop;
    boost::thread* m_thread;
    boost::mutex m_queue_mutex;
    boost::condition_variable m_queue_wake;
    std::queue< boost::function< void() > > m_queue;

    /// ] todo: put into its own class

    boost::shared_ptr<class SimilarArtists> m_sa;
    boost::shared_ptr<class BoffinDb> m_db;
    
    playdar::pa_ptr m_pap;
};

EXPORT_DYNAMIC_CLASS( boffin )


#endif

