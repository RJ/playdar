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
    CometSession(const std::string& session, Resolver* resolver)
        : m_session(session)
        , m_resolver(resolver)
        , m_firstWrite(true)
        , m_cancelled(false)
        , m_writing(false)
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
        m_cancelled = true;
    }

    // serialise into buffer
    void result_item_cb(const query_uid& qid, ri_ptr rip)
    {
        if (!m_cancelled) {
            json_spirit::Object o;
            o.push_back( json_spirit::Pair("query", qid) );
            o.push_back( json_spirit::Pair("result", rip->get_json()) );
            enqueue(json_spirit::write(o), true);
        }
    }

    bool async_write_func(moost::http::reply::WriteFunc& wf)
    {
        m_wf = wf;

        if (!wf || m_cancelled) {   
            std::cout << "comet session " << m_session << " async write ending" << std::endl;
            // cancelled by caller || cancelled by us
            disconnect_from_resolver();
            m_cancelled = true;
            m_wf = 0;
            return false;
        }

        if (m_firstWrite) {
            m_firstWrite = false;
            enqueue("[");        // we're writing an array of javascript objects
            return true;
        }

        {
            boost::lock_guard<boost::mutex> lock(m_mutex);

            if (m_writing && m_buffers.size()) {
                // previous write has completed:
                m_buffers.pop_front();
                m_writing = false;
            }

            if (!m_writing && m_buffers.size()) {
                // write something new
                m_writing = true;
                m_wf(
                    moost::http::reply::async_payload(
                        boost::asio::const_buffer(m_buffers.front().data(), m_buffers.front().length())));
            }
        }
        return true;
    }

private:
    void enqueue(const std::string& s, bool withComma = false)
    {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        m_buffers.push_back(s);
        if (withComma) {
            static std::string comma(",\r\n");
            m_buffers.push_back(comma);
        }
        if (!m_writing) {
            m_writing = true;
            m_wf(
                moost::http::reply::async_payload(
                    boost::asio::const_buffer(m_buffers.front().data(), m_buffers.front().length())));
        }
    }

    std::string m_session;
    Resolver* m_resolver;

    moost::http::reply::WriteFunc m_wf;     // keep a hold of the write func to keep the connection alive.
    bool m_firstWrite;
    bool m_cancelled;

    boost::mutex m_mutex;
    bool m_writing;
    std::list<std::string> m_buffers;
};

}

#endif
