#ifndef __DELIVERY_STRATEGY_H__
#define __DELIVERY_STRATEGY_H__

#include <string>
#include <boost/shared_ptr.hpp>

// Resolvers attach streaming strategies to playable items.
// they are responsible for getting the bytes from the audio file, which
// may be on local disk, or remote HTTP, or something more exotic -
// depends on the resolver.
class StreamingStrategy
{
public:
    StreamingStrategy(){}
    virtual ~StreamingStrategy(){}
    virtual int read_bytes(char * buffer, size_t size) = 0;
    virtual std::string debug() = 0;
    virtual void reset() = 0;
    /// called when we want to use a SS to stream.
    /// could make a copy if the implementation requires it.
    virtual boost::shared_ptr<StreamingStrategy> get_instance()
    {
        return boost::shared_ptr<StreamingStrategy>(this);
    }
};

#endif
