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

namespace playdar {

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
    virtual std::string mime_type() = 0;
    virtual int content_length(){ return -1; }
    virtual void set_extra_header(const std::string& header){}
    /// called when we want to use a SS to stream.
    /// could make a copy if the implementation requires it.
    virtual boost::shared_ptr<StreamingStrategy> get_instance()
    {
        return boost::shared_ptr<StreamingStrategy>(this);
    }

    virtual bool async_delegate(boost::function< void(boost::asio::const_buffer) > writefunc){}; //TODO should be =0
};

}

#endif
