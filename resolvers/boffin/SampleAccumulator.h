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
#ifndef _BOFFIN_SampleAccumulator_H_
#define _BOFFIN_SampleAccumulator_H_

#include <deque>
#include <vector>
#include "ResultSet.h"

class SampleAccumulator 
{
public:
    SampleAccumulator(size_t memory) 
        : m_memory(memory) 
    {
    }

    // return the pushdown factor for this track
    float pushdown(const TrackResult& r)
    {
        for (std::deque<int>::const_iterator it = m_recent.begin(); it != m_recent.end(); it++) {
            if (*it == r.artistId) 
                return 0.00001;
        }
        return 1.0;
    }

    void result(const TrackResult& r)
    {
        while (m_recent.size() > m_memory)
            m_recent.pop_back();
        m_recent.push_front(r.artistId);
        m_results.push_back( r );
    }

    const std::vector<TrackResult>& get_results()
    {
        return m_results;
    }

private:
    size_t m_memory;
    std::deque<int> m_recent;
    std::vector<TrackResult> m_results;
};

#endif