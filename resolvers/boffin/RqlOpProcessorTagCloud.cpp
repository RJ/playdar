/***************************************************************************
 *   Copyright 2007-2009 Last.fm Ltd.                                      *
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


#include "RqlOpProcessorTagCloud.h"
#include "parser/enums.h"
#include "playdar/library.h"
#include "SimilarArtists.h"
#include <boost/bind.hpp>
#include "sqlite3pp.h"

using namespace std;
using namespace fm::last::query_parser;


RqlOpProcessorTagCloud::RqlOpProcessorTagCloud(RqlOpProcessorTagCloud::Iterator begin, RqlOpProcessorTagCloud::Iterator end, BoffinDb& library, SimilarArtists& similarArtists)
    : m_library(library)
    , m_similarArtists(similarArtists)
    , m_it(begin)
    , m_end(end)
{
}

void 
RqlOpProcessorTagCloud::next()
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
RqlOpProcessorTagCloud::process(Params &params)
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

    throw "unknown field";
}


string
RqlOpProcessorTagCloud::globalTag(Params& params)
{
    params.push_back(m_it->name);
    return 
        "SELECT track FROM track_tag "
        "INNER JOIN tag ON tag.rowid = track_tag.tag "
        "WHERE name = ?";
}

string
RqlOpProcessorTagCloud::userTag(Params& params)
{
    // todo: we have no user tags yet
    return globalTag(params);
}

string
RqlOpProcessorTagCloud::artist(Params& params)
{
    // todo
    return "";
}

string
RqlOpProcessorTagCloud::similarArtist(Params& params)
{
    // todo
    return "";
}

// static
RqlOpProcessorTagCloud::TagCloudVecP
RqlOpProcessorTagCloud::process(RqlOpProcessorTagCloud::Iterator begin, 
                                RqlOpProcessorTagCloud::Iterator end, 
                                BoffinDb& library, 
                                SimilarArtists& similarArtists)
{
    Params params;

    string queryString(
    "SELECT name, sum(weight), count(weight) "
    "FROM track_tag "
    "INNER JOIN tag ON track_tag.tag = tag.rowid "
    "WHERE track "
    "IN (" + 
    RqlOpProcessorTagCloud(begin, end, library, similarArtists).process(params) + 
    ") GROUP BY tag.rowid");

    //cout << queryString << endl;

    sqlite3pp::query qry(library.db(), queryString.data());
    int i = 1;
    BOOST_FOREACH(const string& s, params) {
        qry.bind(i, s.data());
        i++;
    }

    TagCloudVecP p( new BoffinDb::TagCloudVec() );
    float maxWeight = 0;
    for(sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
        p->push_back( i->get_columns<string, float, int>(0, 1, 2) );
        maxWeight = max( maxWeight, p->back().get<1>() );
    }

    return p;
}

