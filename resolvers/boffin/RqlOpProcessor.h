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

#ifndef RQL_OP_PROCESSOR_H
#define RQL_OP_PROCESSOR_H

#include "RqlOp.h"
#include "ResultSet.h"

template<typename Iterator>
class RqlOpProcessor
{
    Iterator m_it, m_end;
    class Library& m_library;
    class SimilarArtists& m_similarArtists;

    RqlOpProcessor(Iterator begin, Iterator end, Library& library, SimilarArtists& similarArtists);
    void next();
    ResultSetPtr process();
    ResultSetPtr unsupported();
    ResultSetPtr globalTag();
    ResultSetPtr userTag();
    ResultSetPtr artist();
    ResultSetPtr similarArtist();

    void normalise(float weight, ResultSetPtr rs);

public:
    static ResultSetPtr process(Iterator begin, Iterator end, Library& library, SimilarArtists& similarArtists);

};

#endif