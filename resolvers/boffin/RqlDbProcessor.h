/*
    Playdar - music content resolver
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

#ifndef RQL_DB_PROCESSOR_H
#define RQL_DB_PROCESSOR_H
 
#include <list>
#include <vector>
#include <string>
#include <boost/function.hpp>
#include "RqlOp.h"
#include "BoffinDb.h"

class SimilarArtists;

// RqlDbProcessor is for constructing a db query against a bunch of 
// tracks extracted using an RQL query
//
// Provide two query strings such that this makes sense:
//      query1 + " WHERE track_tag.track IN ( ) " + query2
//
// RqlDbProcessor fills in the track ids from the RQL
// and returns a query
//
class RqlDbProcessor
{
public:
    typedef std::vector<RqlOp>::const_iterator Iterator;
    typedef boost::shared_ptr<sqlite3pp::query> QueryPtr;

    // throws runtime_error on parsing problem
    static 
    QueryPtr 
    parseAndProcess(
        const std::string& rql, 
        const std::string& query1, const std::string& query2,
        BoffinDb& library, SimilarArtists& similarArtists);

    static 
    QueryPtr 
    process(
        Iterator begin, Iterator end, 
        const std::string& query1, const std::string& query2, 
        BoffinDb& library, SimilarArtists& similarArtists);

private:
    typedef std::list<std::string> Params;

    Iterator m_it, m_end;
    BoffinDb& m_library;
    SimilarArtists& m_similarArtists;
 
    RqlDbProcessor(Iterator begin, Iterator end, BoffinDb& library, SimilarArtists& similarArtists);
    void next();
    std::string process(Params &p);
    std::string globalTag(Params &p);
    std::string userTag(Params &p);
    std::string artist(Params &p);
    std::string similarArtist(Params &p);
};


#endif
