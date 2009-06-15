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


#include "RqlOpProcessorTagCloud.h"
#include "parser/enums.h"
#include "parser/parser.h"
#include "../local/library.h"
#include "SimilarArtists.h"
#include <boost/bind.hpp>
#include "sqlite3pp.h"

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



RqlDbProcessor::RqlDbProcessor(RqlDbProcessor::Iterator begin, RqlDbProcessor::Iterator end, BoffinDb& library, SimilarArtists& similarArtists)
    : m_library(library)
    , m_similarArtists(similarArtists)
    , m_it(begin)
    , m_end(end)
{
}

void 
RqlDbProcessor::next()
{
    if (++m_it == m_end) {
        // how could the parser do this to us?
        throw std::runtime_error("unterminated rql");
    }
}

string
string_join(const string& a, const string& b, const string& separator)
{
    if (a.length()) {
        if (b.length()) {
            return a + separator + b;
        }
        return a;
    } 
    return b;
}


// build up a big query by componding select statements like.
// appends parameters to params
string 
RqlDbProcessor::process(Params &params)
{
    if (m_it->isRoot) {
        // note the operator type, then move on and get the operands
        int op = m_it->type;
        string a( (next(), process(params)) );
        string b( (next(), process(params)) );

        switch (op) {
            case OT_AND:
                return string_join(a, b, " INTERSECT ");

            case OT_OR:
                return string_join(a, b, " UNION ");

            case OT_AND_NOT:
                return string_join(a, b, " EXCEPT ");
        }
        throw std::runtime_error("unknown rql operation");
    }

    // it's a leaf node, or a 'source' if you like
    switch (m_it->type) {
        case RS_LIBRARY:
        case RS_LOVED:
        case RS_NEIGHBORS:
        case RS_PLAYLIST:
        case RS_RECOMMENDED:
        case RS_GROUP:
        case RS_EVENT:
            return "";  // unsupported

        case RS_SIMILAR_ARTISTS:
            return similarArtist(params);

        case RS_GLOBAL_TAG:
            return globalTag(params);

        case RS_USER_TAG:
            return userTag(params);

        case RS_ARTIST:
            return artist(params);
    }

    throw std::runtime_error("unknown rql field");
}


string
RqlDbProcessor::globalTag(Params& params)
{
    params.push_back(m_it->name);
    return 
        "SELECT track FROM track_tag "
        "INNER JOIN tag ON tag.rowid = track_tag.tag "
        "WHERE name = ?";
}

string
RqlDbProcessor::userTag(Params& params)
{
    // todo: we have no user tags yet
    return globalTag(params);
}

string
RqlDbProcessor::artist(Params& params)
{
    // todo
    return "";
}

string
RqlDbProcessor::similarArtist(Params& params)
{
    // todo
    return "";
}

//static 
RqlDbProcessor::QueryPtr
RqlDbProcessor::parseAndProcess(
    const std::string& rql, 
    const std::string& query1, const std::string& query2,
    BoffinDb& library, SimilarArtists& similarArtists)
{
    parser p;
    if (!p.parse(rql)) {
        throw std::runtime_error("rql parse error");
    }

    std::vector<RqlOp> ops;
    p.getOperations<RqlOp>(
        boost::bind(&std::vector<RqlOp>::push_back, boost::ref(ops), _1),
        &root2op, 
        &leaf2op);
    return process(
        ops.begin(), ops.end(), 
        query1, query2,
        library, similarArtists);
}

//static 
RqlDbProcessor::QueryPtr 
RqlDbProcessor::process(
    Iterator begin, Iterator end, 
    const std::string& query1, const std::string& query2, 
    BoffinDb& library, SimilarArtists& similarArtists)
{
    Params params;
    string sql = 
        query1 + " WHERE track_tag.track IN (" + 
        RqlDbProcessor(begin, end, library, similarArtists).process(params) + 
        ") " + query2;

    //cout << sql << endl;

    QueryPtr qry(new sqlite3pp::query(library.db(), sql.data()));
    int i = 1;
    BOOST_FOREACH(const string& s, params) {
        qry->bind(i, s.data());
        i++;
    }
    return qry;
}

