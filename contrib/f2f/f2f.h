#ifndef __RS_f2f_DL_H__
#define __RS_f2f_DL_H__

#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <map>
#include <string>

#include "playdar/playdar_plugin_include.h"

#include "jbot.h"

namespace playdar {
namespace resolvers {

class f2f : public ResolverPlugin<f2f>
{
    public:
    f2f(){}
    
    virtual bool init(pa_ptr pap);

    void run();
    void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    void cancel_query(query_uid qid);
    
    std::string name() const { return "f2f"; }
    
    /// max time in milliseconds we'd expect to have results in.
    unsigned int target_time() const
    {
        return 5000;
    }
    
    /// highest weighted resolverservices are queried first.
    unsigned short weight() const
    {
        return 25;
    }
        
    bool anon_http_handler(const playdar_request&, playdar_response& );
    
protected:    
    virtual ~f2f() throw();
    
private:

    pa_ptr m_pap;

    boost::shared_ptr< jbot > m_jbot;
    boost::shared_ptr< boost::thread > m_jbot_thread;

    //boost::shared_ptr< boost::asio::io_service > m_io_service;
    //boost::shared_ptr< boost::thread > m_responder_thread;
};

EXPORT_DYNAMIC_CLASS( f2f )


}}
#endif
