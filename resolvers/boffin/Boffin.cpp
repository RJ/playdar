#include "Boffin.h"


Boffin::Boffin()
    : m_thread( 0 )
    , m_thread_stop( false )
{
}

std::string 
Boffin::name() const
{
    return "Boffin";
}

// return false to disable resolver
bool 
Boffin::init(playdar::Config* c, Resolver* r)
{
    if (!ResolverServicePlugin::init(c, r))
        return false;

    m_thread = new boost::thread( boost::bind(&Boffin::thread_run, this) );
    return true;
}

void
Boffin::queue_work(boost::function< void() > work)
{
    if (!m_thread_stop) 
    {
        {
            boost::lock_guard<boost::mutex> guard(m_queue_mutex);
            m_queue.push(work);
        }
        m_queue_wake.notify_one();
    }
}

boost::function< void() > 
Boffin::get_work()
{
    boost::unique_lock<boost::mutex> lock(m_queue_mutex);
    while (m_queue.empty()) {
        m_queue_wake.wait(lock);
    }
    boost::function< void() > result = m_queue.front();
    m_queue.pop();
    return result;
}

void
Boffin::drain_queue()
{
    boost::lock_guard<boost::mutex> guard(m_queue_mutex);
    while (!m_queue.empty()) {
        m_queue.pop();
    }
}

void
Boffin::thread_run()
{
    try {
        while (!m_thread_stop) {
            get_work()();
        }
    } catch (boost::thread_interrupted) {
        std::cout << "Boffin::thread_run exiting normally";
    } catch (...) {
        std::cout << "Boffin::thread_run unhandled exception";
    }

    drain_queue();
}

void
Boffin::stop()
{
    if (m_thread) {
        m_thread_stop = true;
        m_thread->interrupt();
        m_thread->join();
        delete m_thread;
        m_thread = 0;
    }
}

/// max time in milliseconds we'd expect to have results in.
unsigned int 
Boffin::target_time() const
{
    // we may have zero results (if it's a query we can't handle), 
    // and we don't want to delay any other resolvers, so:
    return 50;
}

/// highest weighted resolverservices are queried first.
unsigned short 
Boffin::weight() const
{
    return 80;
}

void 
Boffin::start_resolving(boost::shared_ptr<ResolverQuery> rq)
{
    queue_work( boost::bind( &Boffin::resolve, this, rq ) );
}

void
Boffin::resolve(boost::shared_ptr<ResolverQuery> rq)
{
    int i = 0;
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
