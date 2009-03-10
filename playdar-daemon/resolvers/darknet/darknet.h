#ifndef __RS_DARKNET_H__
#define __RS_DARKNET_H__
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "boost/bind.hpp"
#include <boost/weak_ptr.hpp>

#include "playdar/playdar_plugin_include.h"

#include "msgs.h"
#include "servent.h"



//namespace playdar {
//namespace resolvers {

namespace playdar { 
namespace resolvers { 

class Servent; //fwd decl

class darknet : public ResolverService
{

public:
    darknet(){};
    
    void init(playdar::Config * c, Resolver * r);
    
    void start_resolving(boost::shared_ptr<ResolverQuery> rq);
    
    std::string name() { return "Darknet"; }
    
    void start_io(boost::shared_ptr<boost::asio::io_service> io_service)
    {
        io_service->run();
        cout << "io_service exiting!" << endl;   // never happens..
    }
    
    /// ---------------------
    
    /// Handle completion of a read operation.
    /// Typically a new message just arrived.
    /// @return true if connection should remain open
    bool handle_read(   const boost::system::error_code& e, 
                        msg_ptr msg, 
                        connection_ptr conn);
                        
    void connection_terminated(connection_ptr conn);
    
    /// called when a msg was sent ok
    void write_completed(connection_ptr conn, msg_ptr msg);
    
    /// called when a client connects to us
    bool new_incoming_connection( connection_ptr conn );
    
    bool new_outgoing_connection( connection_ptr conn, boost::asio::ip::tcp::endpoint &endpoint);
    
    void send_identify(connection_ptr conn );
    
    void start_search(msg_ptr msg);
    
    void fwd_search(const boost::system::error_code& e, 
                    connection_ptr conn, 
                    msg_ptr msg, 
                    boost::shared_ptr<boost::asio::deadline_timer> t, 
                    query_uid qid);
    
    bool handle_searchquery(connection_ptr, msg_ptr msg);
    
    bool handle_searchresult(connection_ptr, msg_ptr msg);
    
    void send_msg(connection_ptr conn, msg_ptr msg);
    
    /// associate username->conn
    void register_connection(string username, connection_ptr conn);
    
    void unregister_connection(string username);
    
    void start_sidrequest(connection_ptr conn, source_uid sid, boost::function<bool (msg_ptr)> handler);
    
    bool handle_sidrequest(connection_ptr conn, msg_ptr msg);
    
    bool handle_siddata(connection_ptr conn, msg_ptr msg);

    boost::shared_ptr<Servent> servent() { return m_servent; }
    
    // callback fired when result available from resolver:
    void send_response( query_uid qid, 
                        boost::shared_ptr<PlayableItem> pip);
    
    void set_query_origin(query_uid qid, connection_ptr conn)
    {
        assert( m_qidorigins.find(qid) == m_qidorigins.end() );
        try
        {
            m_qidorigins[qid] = connection_ptr_weak(conn);
        }
        catch(...){}
    }
    
    connection_ptr get_query_origin(query_uid qid)
    {
        connection_ptr conn;
        if(m_qidorigins.find(qid) == m_qidorigins.end())
        {
            // not found
            return conn;
        }
        try
        {
            connection_ptr_weak connw = m_qidorigins[qid];
            return connection_ptr(connw);
        }catch(...)
        { return conn; }
    }

protected:
    ~darknet() throw() {}
    
private:
    boost::shared_ptr<boost::asio::io_service> m_io_service;
    boost::shared_ptr<boost::asio::io_service> m_io_service_p;
    boost::shared_ptr<Servent> m_servent;
    boost::shared_ptr<boost::asio::io_service::work> m_work;
    // source of queries, so we know how to reply.
    map< query_uid, connection_ptr_weak > m_qidorigins;
    map< source_uid,  boost::function<bool (msg_ptr)> > m_sidhandlers;
    /// Keep track of username -> connection
    /// this tells us how many connections are established, and to whom.
    map<string, connection_ptr_weak> m_connections;
    
};


}}

#endif
