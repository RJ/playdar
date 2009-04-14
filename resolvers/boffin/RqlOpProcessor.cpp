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


RqlOpProcessor::RqlOpProcessor(RqlOpProcessor::Iterator begin, RqlOpProcessor::Iterator end, BoffinDb& library, SimilarArtists& similarArtists)
    : m_library(library)
    , m_similarArtists(similarArtists)
    , m_it(begin)
    , m_end(end)
{
}

void 
RqlOpProcessor::next()
{
    if (++m_it == m_end) {
        throw "unterminated query. how could the parser do this to us??"; // it's an outrage
    }
}

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
                {
                    ResultSetPtr c( new ResultSet );
                    setop::and(*a, *b, *c);
                    return c;
                }

            case OT_OR:
                // a = a union b (with intersection boost factor)
                {
                    const int intersection_boost = 100;
                    for (ResultSet::const_iterator pb = b->begin(); pb != b->end(); pb++) {
                        ResultSet::iterator pa( a->find(*pb) );
                        if (pa == a->end()) {
                            a->insert(*pb);
                        } else {
                            // this is safe (TrackResult::trackId is the set key)
                            pa->weight = intersection_boost * (pb->weight + pa->weight);
                        }
                    }
                }
                return a;

            case OT_AND_NOT:
                {
                    ResultSetPtr c( new ResultSet );
                    setop::and_not(*a, *b, *c);
                    return c;
                }
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

ResultSetPtr
RqlOpProcessor::unsupported()
{
    ResultSetPtr r(new ResultSet());
    return r;
}

void insertTrackResult(ResultSet* rs, int track, int artist, float weight)
{
    rs->insert(TrackResult(track, artist, weight));
}

ResultSetPtr
RqlOpProcessor::globalTag()
{
    ResultSetPtr rs( new ResultSet() );
    m_library.files_with_tag(
        m_it->name, 
        boost::bind( &insertTrackResult, rs.get(), _1, _2, _3 ) );
    normalise(m_it->weight, rs);
    return rs;
}

ResultSetPtr
RqlOpProcessor::userTag()
{
    // todo: we have no user tags yet
    return globalTag();
}

ResultSetPtr
RqlOpProcessor::artist()
{
    ResultSetPtr rs( new ResultSet() );
    m_library.files_by_artist( 
        m_it->name, 
        boost::bind( &insertTrackResult, rs.get(), _1, _2, 1.0f) );
    normalise(m_it->weight, rs); 
    return rs;
}

ResultSetPtr
RqlOpProcessor::similarArtist()
{
    ResultSetPtr rs( m_similarArtists.filesBySimilarArtist(m_library, m_it->name.data()) );
    normalise(m_it->weight, rs);
    return rs;
}

// static
ResultSetPtr
RqlOpProcessor::process(Iterator begin, Iterator end, BoffinDb& library, SimilarArtists& similarArtists)
{
    return RqlOpProcessor(begin, end, library, similarArtists).process();
}

void
RqlOpProcessor::normalise(float weight, ResultSetPtr rs)
{
    if (weight < 0.0001) return;

    ResultSet::iterator p;

    float sum = 0;
    for(p = rs->begin(); p != rs->end(); p++) {
        sum += p->weight;
    }
    sum /= weight;
    if (sum < 0.0001) return;

    // need to cast away the constness to mutate the weight
    // this is safe because the set is not keyed on the weight
    for(p = rs->begin(); p != rs->end(); p++) {
        const_cast<TrackResult*>(&(*p))->weight /= sum;
    }
}
