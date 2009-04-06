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

#ifndef RQL_QUERY_H
#define RQL_SUERY_H

#include "SimilarArtists.h"
#include "../ILocalRql.h"
#include <QQueue>


class RqlQuery : public ILocalRqlTrackSource
{
    class RqlQueryThread* m_queryThread;
    ResultSet m_tracks;
    QQueue<uint> m_artistHistory;

private:
    float pushdownFactor(const QSet<uint>& recentTracks, uint artistId, uint trackId);

public:
    RqlQuery(class RqlQueryThread* queryThread, ResultSet tracks);

    void getNextTrack(LocalCollection&, ILocalRqlTrackCallback*, QSet<uint>& recentTracks);
    unsigned tracksLeft();

    // ILocalRqlTrackSource
    virtual void getNextTrack(ILocalRqlTrackCallback*);
    virtual void finished();
};

#endif