#ifndef __RS_f2f_DL_H__
#define __RS_f2f_DL_H__

#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <iostream>
#include <map>
#include <string>

#include "playdar/playdar_plugin_include.h"

#include "jbot.h"

namespace playdar {
namespace resolvers {

/// XMPP/Jabber resolver, starts a jabber bot (jbot class) and will detect other
/// people on your roster with playdar support (ie, running this code too)
/// and send searches to them, and handle incoming replies to queries.
class f2f : public ResolverPlugin<f2f>
{
    public:
    f2f(){}
    
    virtual bool init(pa_ptr pap);
    void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    void cancel_query(query_uid qid);
    std::string name() const { return "f2f"; }
    unsigned int target_time() const { return 5000; }
    unsigned short weight() const { return 25; }
    bool anon_http_handler(const playdar_request&, playdar_response& );
    
    void msg_runner();
    void msg_received( const std::string& msg, const std::string& from );
    void send_response( query_uid qid, ri_ptr rip, const std::string& from );
    
protected:    
    virtual ~f2f() throw();
    
private:

    pa_ptr m_pap;

    boost::shared_ptr< jbot > m_jbot;
    boost::shared_ptr< boost::thread > m_jbot_thread;

    boost::shared_ptr< boost::thread > m_msg_thread;
    std::deque<rq_ptr> m_msg_pending;
    boost::mutex m_msg_mutex;
    boost::condition m_msg_cond;
    bool m_exiting;
};

EXPORT_DYNAMIC_CLASS( f2f )


}}
#endif
