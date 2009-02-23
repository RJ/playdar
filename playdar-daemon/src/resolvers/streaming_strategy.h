#ifndef __DELIVERY_STRATEGY_H__
#define __DELIVERY_STRATEGY_H__

// Resolvers attach streaming strategies to playable items.
// they are responsible for getting the bytes from the audio file, which
// may be on local disk, or remote HTTP, or something more exotic -
// depends on the resolver.
class StreamingStrategy
{
public:
    StreamingStrategy(){}
    virtual ~StreamingStrategy(){}
    virtual int read_bytes(char * buffer, int size) = 0;
    virtual string debug() = 0;
    virtual void reset() = 0;
};

#endif
