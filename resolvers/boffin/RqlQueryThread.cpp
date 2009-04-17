/***************************************************************************
 *   Copyright 2005-2009 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/


#include "RqlQueryThread.h"
#include "LocalCollection.h"
#include "QueryError.h"
#include <QDebug>

#include "RqlQuery.h"

#include "RqlOpProcessor.h"
#include "rqlparser/parser.h"
#include <boost/bind.hpp>
#include <sstream>
using namespace std;
using namespace fm::last::query_parser;


static RqlOp root2op( const querynode_data& node )
{
   RqlOp op;
   op.isRoot = true;
   op.name = node.name;
   op.type = node.type;
   op.weight = node.weight;

   return op;
}


static RqlOp leaf2op( const querynode_data& node )
{
   RqlOp op;
   op.isRoot = false;

   if ( node.ID < 0 )
      op.name = node.name;
   else
   {
      ostringstream oss;
      oss << '<' << node.ID << '>';
      op.name = oss.str();
   }
   op.type = node.type;
   op.weight = node.weight;

   return op;
}


void 
RqlQueryThread::run()
{
    try {
        m_pCollection = LocalCollection::create("RqlQueryThread");
        TRequestThread<QueryRunnable>::run();
    } 
    catch (QueryError &e) {
        qCritical() << "RqlQueryThread::run: " + e.text();
    }
    catch (...) {
        qCritical() << "RqlQueryThread::run: unhandled exception";
    }

    try {
        // finish any remaining requests without actually doing anything
        QueryRunnable* req;
        while (0 != (req = takeNextFromQueue())) {
            req->abort();
            delete req;
        }

        delete m_pCollection;
        m_pCollection = 0;
    }
    catch (...) {
        qCritical() << "QueryThread::run: unhandled exception";
    }
}

void
RqlQueryThread::doRequest(QueryRunnable* runnable)
{
    runnable->doRequest(this);
    delete runnable;
}

void
RqlQueryThread::parse(QString rql, ILocalRqlParseCallback *cb)
{
	try 
	{
        parser p;
        string srql(rql.toUtf8());
        if (p.parse(srql)) {
            QList<RqlOp> ops;

            p.getOperations<RqlOp>(
                boost::bind(&QList<RqlOp>::push_back, boost::ref(ops), _1),
                &root2op, 
                &leaf2op);
            
            ResultSet results = RqlOpProcessor::process(ops, *m_pCollection, m_sa);
            if (rql != m_recentRql) {
                m_recentRql = rql;
                m_recentTracks.clear();
            }
            cb->parseOk(
                new RqlQuery(this, results),
                results.size() );
            return;
        } 
        cb->parseFail(
            p.getErrorLineNumber(),
            p.getErrorLine().data(),
            p.getErrorOffset() );
        return;
	} 
	catch(QueryError &e) {
        qWarning() << "RqlQueryThread::parse: " + e.text();
	}
    catch(...) {
        qWarning() << "RqlQueryThread::parse: unhandled exception";
    }

    cb->parseFail(0, "unhandled exception", 0);
}

void
RqlQueryThread::nextTrack(RqlQuery* query, ILocalRqlTrackCallback* cb)
{
	try 
	{
        query->getNextTrack(*m_pCollection, cb, m_recentTracks);
        return;
    }
	catch(QueryError &e) {
        qWarning() << "RqlQueryThread::nextTrack: " + e.text();
	}
    catch(...) {
        qWarning() << "RqlQueryThread::nextTrack: unhandled exception";
    }

    cb->trackFail();
}

void
RqlQueryThread::enqueueParse(const char* sRql, ILocalRqlParseCallback *cb)
{
    enqueue(new ParseRunnable(QString(sRql), cb));
}

void
RqlQueryThread::enqueueGetNextTrack(RqlQuery* src, ILocalRqlTrackCallback* cb)
{
    enqueue(new NextTrackRunnable(src, cb));
}

void
RqlQueryThread::enqueueDelete(class RqlQuery* q)
{
    enqueue(new DeleteRunnable(q));
}

/////////////////////////////////////////////////////////////////////


RqlQueryThread::ParseRunnable::ParseRunnable(QString rql, ILocalRqlParseCallback *cb)
: m_rql(rql)
, m_cb(cb)
{
}

void
RqlQueryThread::ParseRunnable::doRequest(RqlQueryThread* thread)
{
    thread->parse(m_rql, m_cb);
}

void
RqlQueryThread::ParseRunnable::abort()
{
    m_cb->parseFail(0, "aborted", 0);
}

////////////


RqlQueryThread::NextTrackRunnable::NextTrackRunnable(RqlQuery* src, ILocalRqlTrackCallback* cb)
: m_src(src)
, m_cb(cb)
{
}

void
RqlQueryThread::NextTrackRunnable::doRequest(RqlQueryThread* thread)
{
    thread->nextTrack(m_src, m_cb);
}

void
RqlQueryThread::NextTrackRunnable::abort()
{
}

//////////////


RqlQueryThread::DeleteRunnable::DeleteRunnable(class RqlQuery* query)
: m_query(query)
{
}

void 
RqlQueryThread::DeleteRunnable::doRequest(class RqlQueryThread*)
{
    delete m_query;
}

void 
RqlQueryThread::DeleteRunnable::abort()
{
    delete m_query;
}

