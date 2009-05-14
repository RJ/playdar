#ifndef COMET_SESSION_H
#define COMET_SESSION_H

#include <boost/thread.hpp>

#include "playdar/types.h"
#include "playdar/resolved_item.h"

// unite the moost::http::reply content_func callback 
// with the ResolverQuery result_item_callback

namespace playdar {

class CometSession
{
public:
    CometSession()
        : m_cancelled(false)
        , m_offset(0)
    {
        enqueue("[");        // we're writing an array of javascript objects
    }

    // cause the content_func to break-out
    void cancel()
    {
        m_cancelled = true;
        signal();
    }

    // serialise into buffer
    // signal to content_func
    void result_item_cb(const query_uid& qid, ri_ptr rip)
    {
        json_spirit::Object o;
        o.push_back( json_spirit::Pair("query", qid) );
        o.push_back( json_spirit::Pair("result", rip->get_json()) );
        enqueue(json_spirit::write_formatted(o));
        enqueue(",");     // comma to separate objects in array
        signal();
    }

    // block in here until we have some data 
    size_t content_func(char* buffer, size_t size)
    {
        if (m_cancelled) return 0;

        boost::unique_lock<boost::mutex> lock(m_mutex);

        if (0 == m_buffers.size()) {
            m_wake.wait(lock);
            if (m_cancelled) return 0;
        }

        size_t result = 0;
        {
            // fill as much of the supplied buffer as we can
            while (size && m_buffers.size()) {
                size_t buf_avail = m_buffers.front().length() - m_offset;
                size_t c = std::min(buf_avail, size);
                memcpy(buffer, m_buffers.front().data() + m_offset, c);
                buffer += c;
                result += c;
                m_offset += c;
                size -= c;
                if (m_offset == m_buffers.front().length()) {
                    m_buffers.pop_front();
                    m_offset = 0;
                }
            }
        }
        return result;
    }

private:
    void signal()
    {
        m_wake.notify_one();
    }

    void enqueue(const std::string& s)
    {
        boost::lock_guard<boost::mutex> lock(m_mutex);
        m_buffers.push_back(s);
    }

    boost::mutex m_mutex;
    boost::condition_variable m_wake;
    std::list<std::string> m_buffers;
    int m_offset;
    volatile bool m_cancelled;
};

}

#endif
