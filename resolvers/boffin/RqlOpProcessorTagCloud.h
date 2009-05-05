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
 
#ifndef RQL_OP_PROCESSOR_TAG_CLOUD_H
#define RQL_OP_PROCESSOR_TAG_CLOUD_H
 
#include <list>
#include <vector>
#include <string>
#include "RqlOp.h"
#include "BoffinDb.h"

class SimilarArtists;

// for getting a tag cloud result from an RQL query
//
class RqlOpProcessorTagCloud
{
public:
    typedef std::vector<RqlOp>::const_iterator Iterator;
    typedef boost::shared_ptr<BoffinDb::TagCloudVec> TagCloudVecP;

    static TagCloudVecP process(Iterator begin, Iterator end, BoffinDb& library, SimilarArtists& similarArtists);
 
private:
    typedef std::list<std::string> Params;

    Iterator m_it, m_end;
    BoffinDb& m_library;
    SimilarArtists& m_similarArtists;
 
    RqlOpProcessorTagCloud(Iterator begin, Iterator end, BoffinDb& library, SimilarArtists& similarArtists);
    void next();
    std::string process(Params &p);
    std::string globalTag(Params &p);
    std::string userTag(Params &p);
    std::string artist(Params &p);
    std::string similarArtist(Params &p);
};
 
#endif
