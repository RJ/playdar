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

namespace playdar {

// StreamingStrategies receive an AsyncAdaptor via start_reply.

class AsyncAdaptor
{
public:
    virtual void set_content_length(int contentLength) = 0;
    virtual void set_mime_type(const std::string& mimetype) = 0;
    virtual void set_status_code(int status) = 0;
    virtual void write_content(const char *buffer, int size) = 0;
    virtual void write_finish() = 0;
    virtual void write_cancel() = 0;
    virtual void set_finished_cb(boost::function<void(void)> cb) = 0;
};

typedef boost::shared_ptr<AsyncAdaptor> AsyncAdaptor_ptr;

// Resolvers attach streaming strategies to playable items.
// they are responsible for getting the bytes from the audio file, which
// may be on local disk, or remote HTTP, or something more exotic -
// depends on the resolver.

class StreamingStrategy
{
public:
    StreamingStrategy(){}
    virtual ~StreamingStrategy(){}
    
    virtual std::string debug() = 0;
    virtual void reset() = 0;

    virtual void set_extra_header(const std::string& header){};

    /// called when we want to use a SS to stream.
    /// could make a copy if the implementation requires it.
    virtual boost::shared_ptr<StreamingStrategy> get_instance()
    {
        return boost::shared_ptr<StreamingStrategy>(this);
    }

    // start_reply returns immediately, then sends content via the adaptor.
    virtual void start_reply(AsyncAdaptor_ptr adaptor) = 0;
};

}

#endif
