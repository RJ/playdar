/*
    Playdar - music content resolver
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
#ifndef HTTP_ASYNC_ADAPTOR
#define HTTP_ASYNC_ADAPTOR

#include "streaming_strategy.h"
#include "moost/http/reply.hpp"

namespace playdar {

// provide an AsyncAdapter for an http response via moost::http
//
class HttpAsyncAdaptor : public AsyncAdaptor
{
public:
    HttpAsyncAdaptor(moost::http::reply_ptr reply)
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
        m_reply->write_release();
        m_reply->write_finish();
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

}

#endif
