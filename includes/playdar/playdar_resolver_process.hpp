/*
    This header is a helper for any external resolver written in c++
    It handles communicating with playdar over stdio.
*/
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <stdio.h>
#include <iostream>
#include <map>
#include <string>

#include "json_spirit/json_spirit.h"
#include "playdar/resolver_query.hpp"
#include "playdar/playable_item.hpp"

#define MAXBUF 4096 // max size of msg payload

using namespace std;
using namespace json_spirit;

set< query_uid > dispatched_qids; // qids we already searched for.

// send a msg back to playdar app
// it takes a JSON val, and prepends the frame (4 byte length indicator)
void playdar_write( Value v )
{
    ostringstream os;
    write_formatted( v, os );
    string msg = os.str();
    boost::uint32_t len = htonl(msg.length());
    cout.write( (char*)&len, 4 );
    cout.write( msg.data(), msg.length() );
    cout.flush();
}

// ask if we should dispatch this query (checks if we've already seen it etc)
bool playdar_should_dispatch( query_uid qid )
{
    if(dispatched_qids.find(qid)!=dispatched_qids.end())
    {
        //cerr << "Skipping dispatch, already seen qid " << rq->id() << endl;
        return false;
    }else{
        dispatched_qids.insert( qid );
        return true;
    }
}

// tell playdar to start resolving this query
void playdar_dispatch( rq_ptr rq )
{
    Object o;
    o.push_back( Pair("_msgtype", "query") );
    o.push_back( Pair("query", rq->get_json()) );
    playdar_write( o );
}


// tell playdar we found a result for something
void playdar_report_results( query_uid qid, vector< pi_ptr > results )
{
    cerr << "Reporting results (not) " << endl;
    Object o;
    o.push_back( Pair("_msgtype", "results") );
    o.push_back( Pair("qid", qid) );
    Array arr;
    BOOST_FOREACH( pi_ptr pip, results )
    {
        arr.push_back( pip->get_json() );
    }
    o.push_back( Pair("results", arr) );
    playdar_write( o );
}

// report settings about this resolver
void playdar_send_settings( const string& name, int weight, int targettime )
{
    Object o;
    o.push_back( Pair("_msgtype", "settings") );
    o.push_back( Pair("name", name) );
    o.push_back( Pair("weight", weight) );
    o.push_back( Pair("targettime", targettime) );
    playdar_write( o );
}




