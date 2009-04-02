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


#include "RqlOpProcessor.h"
#include "parser/enums.h"
#include "playdar/library.h"
#include "SimilarArtists.h"
#include <boost/bind.hpp>


using namespace fm::last::query_parser;

template<typename Iterator>
RqlOpProcessor::RqlOpProcessor(Iterator begin, Iterator end, Library& library, SimilarArtists& similarArtists)
    : m_library(library)
    , m_similarArtists(similarArtists)
    , m_it(begin)
    , m_end(end)
{
}

template<typename Iterator>
void 
RqlOpProcessor::next()
{
    if (++m_it == m_end) {
        throw "unterminated query. how could the parser do this to us??"; // it's an outrage
    }
}

template<typename Iterator>
ResultSetPtr 
RqlOpProcessor::process()
{
    if (m_it->isRoot) {
        // note the operator type, then move on and get the operands
        int op = m_it->type;
        ResultSetPtr a( (next(), process()) );
        ResultSetPtr b( (next(), process()) );
        switch (op) {
            case OT_AND:
                setop::and(*a, *b);
                return a;

            case OT_OR:
                setop::or(*a, *b);
                return a;

            case OT_AND_NOT:
                setop::and_not(*a, *b);
                return a;
        }
        throw "unknown operation";
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
            return unsupported();

        case RS_SIMILAR_ARTISTS:
            return similarArtist();

        case RS_GLOBAL_TAG:
            return globalTag();

        case RS_USER_TAG:
            return userTag();

        case RS_ARTIST:
            return artist();
    }

    throw "unknown field";
}

template<typename Iterator>
ResultSetPtr
RqlOpProcessor::unsupported()
{
    ResultSetPtr r(new ResultSet());
    return r;
}

ResultSetPtr
RqlOpProcessor::globalTag()
{
    ResultSetPtr rs( new ResultSet() );
    m_library.filesWithTag(
        m_it->name.data(), 
        LocalCollection::AvailableSources, 
        boost::bind( &ResultSet::insertTrackResult, rs.get(), _1, _2, _3 ) );
    normalise(m_it->weight, rs);
    return rs;
}

template<typename Iterator>
ResultSetPtr
RqlOpProcessor::userTag()
{
    // todo: we have no user tags yet
    return globalTag();
}

void insertTrackResult(ResultSet* rs, int track, int artist, float weight)
{
    rs.insert(new TrackResult(track, artist, weight));
}

template<typename Iterator>
ResultSetPtr
RqlOpProcessor::artist()
{
    ResultSetPtr rs( new ResultSet() ):
    m_collection.filesByArtist( 
        m_it->name.data(), 
        LocalCollection::AvailableSources,
        boost::bind( &insertTrackResult, rs.get(), _1, _2, 1.0f) );
    normalise(m_it->weight, rs); 
    return rs;
}

template<typename Iterator>
ResultSet 
RqlOpProcessor::similarArtist()
{
    ResultSetPtr rs( m_similarArtists.filesBySimilarArtist(m_collection, m_it->name.data()) );
    normalise(m_it->weight, rs);
    return rs;
}

// static
template<typename Iterator>
ResultSetPtr
RqlOpProcessor::process(Iterator begin, Iterator end, LocalCollection& collection, SimilarArtists& similarArtists)
{
    return RqlOpProcessor(begin, end, collection, similarArtists).process();
}

void
RqlOpProcessor::normalise(float weight, ResultSetPtr rs)
{
    if (weight < 0.0001) return;

    float sum = 0;
    foreach(const TrackResult& tr, rs) {
        sum += tr.weight;
    }
    sum /= weight;
    if (sum < 0.0001) return;

    for(ResultSet::iterator p = rs.begin(); p != rs.end(); p++) {
        const_cast<TrackResult*>(&(*p))->weight /= sum;
    }
}
