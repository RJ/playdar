/*
    Playdar - music content resolver
    Copyright (C) 2009  Richard Jones
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
#ifndef __LOCAL_FILE_STRAT_H__
#define __LOCAL_FILE_STRAT_H__

#include <fstream>  
#include <sstream>
#include <iostream>

#include <boost/filesystem.hpp> 

namespace playdar {

class LocalFileStreamingStrategy : public StreamingStrategy
{
public:

    LocalFileStreamingStrategy(const std::string& p)
        : m_uri(p)
    {
        m_connected=false;
    }
    
    ~LocalFileStreamingStrategy()
    {
        reset();
    }
    
    size_t read_bytes(char * buf, size_t size)
    {
        if(!m_connected) do_connect();
        if(!m_connected) return 0;
        size_t len = m_is.read(buf, size).gcount();
        if(len==0) reset();
        return m_is.gcount();
    }
    
    std::string debug()
    { 
        std::ostringstream s;
        s << "LocalFileStreamingStrategy(" << m_uri << ")";
        return s.str();
    }

    void reset()
    {
        if(m_is.is_open()) m_is.close();
        m_connected = false;
    }
    
private:

    void do_connect()
    {
        reset();
        //cout << debug() << endl;
        m_is.open(m_uri.c_str(), std::ios::in | std::ios::binary);
        if(!m_is.is_open())
        {
            std::cout << "Failed to open file: " << m_uri << std::endl;
            return;
        }
        m_connected = true;
    }
    
    std::string m_uri;
    std::ifstream m_is;
    bool m_connected;
};

}

#endif
