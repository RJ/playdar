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
const boost::uint32_t    SIDDATA         = 7;


// generic "empty" uuid. no special significance.
const char empty_msg_id[36] = {0}; 

// these frame every message on the wire:
struct msg_header {
    boost::uint32_t msgtype;
    boost::uint32_t length;
    char id[36];
};

// serialized msg on the wire to this:
struct wire_message {
    msg_header header;
    string payload;
};

class Connection;
typedef boost::shared_ptr<Connection> connection_ptr;
class LameMsg;
typedef boost::shared_ptr<LameMsg> msg_ptr;


// all messages for the wire must be of this type:
class LameMsg {
    public:
            //LameMsg(){}
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
            
            bool serialize(std::string *s) const
            { 
                *s = string(m_payload); 
                return true; 
            }
            static msg_ptr unserialize(msg_header header, string payload)
            {
                msg_ptr m(new LameMsg(payload, header.msgtype));
                return m;
            }
            string toString() const
            {
                ostringstream s;
                s   << "[msg/" << msgtype() << "/" << m_payload << "]";
                return s.str();
            }
     private:
        boost::uint32_t m_msgtype;
        string m_payload;
};



}} //namespaces
#endif
