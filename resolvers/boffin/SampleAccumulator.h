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