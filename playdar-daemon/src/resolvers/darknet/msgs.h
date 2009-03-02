#ifndef _PLAYDAR_DARKNET_MSGS_HPP_
#define _PLAYDAR_DARKNET_MSGS_HPP_
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <vector>
using namespace std;

namespace playdar {
namespace darknet {

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
    string payload;
};

// msgs of type SIDDATA have this header at the start of the payload:
struct sid_header {
    char sid[36];
};

class Connection;
typedef boost::shared_ptr<Connection> connection_ptr;
class LameMsg;
typedef boost::shared_ptr<LameMsg> msg_ptr;


// all messages for the wire must be of this type:
class LameMsg {
    public:
            LameMsg(){}
            LameMsg(string s) 
                : m_payload(s) 
            {m_msgtype=GENERIC;}
            LameMsg(string s, const boost::uint32_t t) 
                : m_payload(s) 
            {m_msgtype = t;}
            
//             ~LameMsg()
//             {
//                 cout << "DTOR:" << toString() << endl;
//             }
            
            boost::uint32_t msgtype() const { return m_msgtype; }
            string payload() const { return m_payload; }
            
            void set_payload(string s) { m_payload = s; }
            
            const std::string & serialize() const
            { 
                return m_payload; 
            }

            string toString(bool shorten=false) const
            {
                ostringstream s;
                s   << "[msg/" << msgtype() << "/" << m_payload.length() << "/" 
                    << ((msgtype()==999/*SIDDATA*/)?"BINARYDATA": (shorten?m_payload.substr(0,50):m_payload)) << "]";
                return s.str();
            }
            
            // buffer used when we marshal this msg and change byteorder:
            msg_header m_outbound_header;
            msg_header m_inbound_header;
            char m_inbound_data[16384]; // 16k buffer max!
            string m_payload;
            boost::uint32_t m_msgtype;
     private:
        
        
};



}} //namespaces
#endif
