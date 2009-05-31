#ifndef COMET_SESSION_H
#define COMET_SESSION_H

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "playdar/types.h"
#include "playdar/resolved_item.h"
#include "moost/http/reply.hpp"

// unite the moost::http::reply async_delegate callback 
// with the ResolverQuery result_item_callback


namespace playdar {

class CometSession : public boost::enable_shared_from_this<CometSession>
{
public:
    CometSession(const std::string& session, moost::http::reply_ptr reply, Resolver* resolver)
        : m_session(session)
        , m_reply(reply)
        , m_resolver(resolver)
        , m_firstWrite(true)
    {
    }

#ifndef NDEBUG
    ~CometSession()
    {
        std::cout << "comet session " << m_session << " deleted" << std::endl;
    }
#endif

    bool connect_to_resolver()
    {
        boost::shared_ptr<CometSession> p(shared_from_this());
        return m_resolver->create_comet_session(
            m_session, 
            boost::bind(&CometSession::result_item_cb, p, _1, _2));
    }

    void disconnect_from_resolver()
    {
        m_resolver->remove_comet_session(m_session);
    }

    // terminate the comet session
    void cancel()
    {
        m_reply->write_cancel();
    }

    // serialise into buffer
    void result_item_cb(const query_uid& qid, ri_ptr rip)
    {
        json_spirit::Object o;
        o.push_back( json_spirit::Pair("query", qid) );
        o.push_back( json_spirit::Pair("result", rip->get_json()) );
        enqueue(json_spirit::write(o), true);
    }


private:
    void enqueue(const std::string& s, bool withComma = false)
    {
        m_reply->write_content(s);
        if (withComma) {
            static std::string comma(",\r\n");
            m_reply->write_content(comma);
        }
    }

    std::string m_session;
    moost::http::reply_ptr m_reply;
    Resolver* m_resolver;
    bool m_firstWrite;
};

}

#endif
