/*
    Playdar - music content resolver
    Copyright (C) 2009  Richard Jones
    Copyright (C) 2009  Last.fm Ltd.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __DELIVERY_STRATEGY_H__
#define __DELIVERY_STRATEGY_H__

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include "moost/http/reply.hpp"

namespace playdar {

// Resolvers attach streaming strategies to playable items.
// they are responsible for getting the bytes from the audio file, which
// may be on local disk, or remote HTTP, or something more exotic -
// depends on the resolver.

class AsyncAdapter
{
public:
    virtual void set_content_length(int contentLength) = 0;
    virtual void set_mime_type(const std::string& mimetype) = 0;
    virtual void write_content(const char *buffer, int size) = 0;
    virtual void write_finish() = 0;
    virtual void write_cancel() = 0;
    virtual void set_finished_cb(boost::function<void(void)> cb) = 0;
};


class HttpAsyncAdapter : public AsyncAdapter
{
public:
    HttpAsyncAdapter(moost::http::reply_ptr reply)
        : m_reply(reply)
        , m_bHaveMimeType(false)
        , m_bHaveContentLength(false)
    {
        m_reply->write_hold();      // hold the content until we get the content_length/mime_type
    }

    virtual void write_content(const char *buffer, int size)
    {
        m_reply->write_content(std::string(buffer, size));
    }

    virtual void write_finish()
    {
        m_reply->write_finish();
    }

    virtual void write_cancel()
    {
        // something went wrong, set the http status code (if it's not too late)
        m_reply->set_status(500);  
        m_reply->write_cancel();
    }

    virtual void set_content_length(int contentLength)
    {
        if (!m_bHaveContentLength && contentLength >= 0) {
            m_reply->add_header("Content-Length", contentLength);
        }
        m_bHaveContentLength = true;
        if (m_bHaveMimeType && m_bHaveContentLength) {
            m_reply->write_release();
        }
    }

    virtual void set_mime_type(const std::string& mimetype)
    {
        if (!m_bHaveMimeType && mimetype.length()) {
            m_reply->add_header("Content-Type", mimetype);
        }
        m_bHaveMimeType = true;
        if (m_bHaveMimeType && m_bHaveContentLength) {
            m_reply->write_release();
        }
    }

    virtual void set_finished_cb(boost::function<void(void)> cb)
    {
        m_reply->set_write_ending_cb(cb);
    }

    moost::http::reply_ptr m_reply;
    bool m_bHaveMimeType;
    bool m_bHaveContentLength;
};

typedef boost::shared_ptr<AsyncAdapter> AsyncAdapter_ptr;


class StreamingStrategy
{
public:
    StreamingStrategy(){}
    virtual ~StreamingStrategy(){}
    
    virtual std::string debug() = 0;
    virtual void reset() = 0;
    virtual std::string mime_type() = 0;
    virtual int content_length(){ return -1; }
    virtual void set_extra_header(const std::string& header){}
    /// called when we want to use a SS to stream.
    /// could make a copy if the implementation requires it.
    virtual boost::shared_ptr<StreamingStrategy> get_instance()
    {
        return boost::shared_ptr<StreamingStrategy>(this);
    }

    virtual void start_reply(AsyncAdapter_ptr adapter) = 0;
};

}

#endif
