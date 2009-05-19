#ifndef COMET_SESSION_H
#define COMET_SESSION_H

#include <boost/thread.hpp>
#include <boost/asio.hpp>

#include "playdar/types.h"
#include "playdar/resolved_item.h"

// unite the moost::http::reply async_delegate callback 
// with the ResolverQuery result_item_callback

namespace playdar {

class CometSession
{
public:
    CometSession()
        : m_cancelled(false)
        , m_writing(false)
        , m_firstWrite(true)
    {
    }

    // terminate the comet session
    void cancel()
    {
        m_cancelled = true;
    }

    // serialise into buffer
    void result_item_cb(const query_uid& qid, ri_ptr rip)
    {
        json_spirit::Object o;
        o.push_back( json_spirit::Pair("query", qid) );
        o.push_back( json_spirit::Pair("result", rip->get_json()) );
        enqueue(json_spirit::write_formatted(o));
        enqueue(",");     // comma to separate objects in array
    }

    typedef boost::function< void(boost::asio::const_buffer)> WriteFunc;
    
    bool async_write_func(WriteFunc& wf)
    {
        if (!wf || m_cancelled) {   // cancelled by caller || cancelled by us
            delete this;
            return false;
        }

        m_wf = wf;
        {
            // previous write has completed:
            boost::lock_guard<boost::mutex> lock(m_mutex);
            if (m_writing && m_buffers.size()) {
                m_buffers.pop_front();
                m_writing = false;
            }
            if (!m_writing && m_buffers.size()) {
                m_writing = true;
                m_wf(boost::asio::const_buffer(m_buffers.front().data(), m_buffers.front().length()));
            }
        }
        return true;
    }

private:
    void enqueue(const std::string& s)
    {
        if (m_firstWrite) {
            m_firstWrite = false;
            enqueue("[");        // we're writing an array of javascript objects
        }

        boost::lock_guard<boost::mutex> lock(m_mutex);
        m_buffers.push_back(s);
        if (!m_writing) {
            m_writing = true;
            m_wf(boost::asio::const_buffer(m_buffers.front().data(), m_buffers.front().length()));
        }
    }

    WriteFunc m_wf;     // keep a hold of the write func to keep the connection alive.
    bool m_firstWrite;
    bool m_cancelled;

    boost::mutex m_mutex;
    bool m_writing;
    std::list<std::string> m_buffers;
};

}

#endif
