#ifndef _PLAYDAR_DARKNET_MSGS_HPP_
#define _PLAYDAR_DARKNET_MSGS_HPP_

#include <vector>
#include <sstream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/weak_ptr.hpp>
#include <iostream>

namespace playdar {
namespace resolvers {

// types of messages:
const boost::uint32_t    GENERIC         = 0;
const boost::uint32_t    WELCOME         = 1;
const boost::uint32_t    IDENTIFY        = 2;
const boost::uint32_t    SEARCHQUERY     = 3;
const boost::uint32_t    SEARCHRESULT    = 4;
const boost::uint32_t    SEARCHCANCEL    = 5;
const boost::uint32_t    CONNECTTO       = 6;
const boost::uint32_t    SIDREQUEST      = 7;
const boost::uint32_t    SIDDATA         = 8;


// generic "empty" uuid. no special significance.
const char empty_msg_id[36] = {0}; 

// these frame every message on the wire:
struct msg_header {
    boost::uint32_t msgtype;
    boost::uint32_t length;
};

// serialized msg on the wire to this:
struct wire_message {
    msg_header header;
    std::string payload;
};

// msgs of type SIDDATA have this header at the start of the payload:
struct sid_header {
    char sid[36];
    char padding[1];
    char debug[6];
};

class Connection;
typedef boost::shared_ptr<Connection> connection_ptr;
typedef boost::weak_ptr<Connection> connection_ptr_weak;
class LameMsg;
typedef boost::shared_ptr<LameMsg> msg_ptr;

// all messages for the wire must be of this type:
class LameMsg {
public:

    LameMsg() : m_payload_len(0)
    {m_msgtype=GENERIC; m_membuf=0;}

    LameMsg(std::string s): m_payload(s), m_payload_len(s.length())
    {m_msgtype=GENERIC; m_membuf=0;}

    LameMsg(std::string s, const boost::uint32_t t)
        : m_payload(s), m_payload_len(s.length())
    {m_msgtype = t; m_membuf=0;}

    LameMsg(const boost::uint32_t t) 
        : m_payload(""), m_payload_len(0) 
    {m_msgtype = t; m_membuf=0;}

    ~LameMsg()
    {
        //cout << "** DTOR:" << toString() << endl;
        free_membuf();
    }

    // anything set here will be free()ed by our destructor:
    void set_membuf(char * k)
    {
        m_membuf = k;
    }
    char * membuf() const{ return m_membuf; }

    void free_membuf()
    {
        if(m_membuf)
        {
            free(m_membuf);
        }
    }

    boost::uint32_t msgtype() const { return m_msgtype; }
    std::string payload() const { return m_payload; }

    void set_payload(std::string s, boost::uint32_t len) 
    { 
        m_payload = s; 
        m_payload_len=len; 
    }

    boost::uint32_t payload_len() const { return m_payload_len; }

    const std::string & serialize() const
    { 
        return m_payload; 
    }

    // used for debugging-by-print-statement, yay
    std::string toString(bool shorten=false) const
    {
        if(msgtype()==SIDDATA) shorten=true;
        std::ostringstream s;
        if(shorten)
        {
            s   << "[msg/" << msgtype() << "/" << m_payload.length() 
                << "/*binary_data*]";
        }else{
            s   << "[msg/" << msgtype() << "/" << m_payload.length() 
                << "/" ;
            s.write(payload().data(), payload_len());
            s << "]\n";
        }
        return s.str();
    }

    // buffer used when we marshal this msg and change byteorder:

    boost::uint32_t m_msgtype;
    boost::uint32_t m_expected_len;
private:
    char * m_membuf;
    std::string m_payload;
    boost::uint32_t m_payload_len;
        
};



}} //namespaces
#endif
