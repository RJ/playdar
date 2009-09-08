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
#ifndef __JSON_CONFIG_HPP__
#define __JSON_CONFIG_HPP__

#include "json_spirit/json_spirit.h"
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <vector>
#include <sstream>
#include <boost/asio/ip/host_name.hpp> // for hostname.
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "boost/filesystem.hpp"

#define DEFAULT_HTTP_PORT 60210

namespace playdar {

// wrapper around JSON config file
// instance of this is also passed to plugins.

class Config
{
public:
    Config(){}
    
    Config(const std::string& f) : m_filename(f)
    {
        reload();
    }
    
    // gets directory where config files are located
    std::string config_dir() const
    {
        boost::filesystem::path p(m_filename);
        return p.parent_path().string();
    }
    
    void reload()
    {
        std::fstream ifs;
        ifs.open(m_filename.c_str(), std::ifstream::in);
        if(ifs.fail())
        {
            throw std::runtime_error("Failed to open config file");
        }
        if(!read(ifs, m_mainval))
        {
            throw std::runtime_error("Failed to parse config file");
        }
        if(m_mainval.type() != json_spirit::obj_type)
        {
            throw("Config file isn't a JSON object!");
        }
        obj_to_map(m_mainval.get_obj(), m_mainmap);
    }
    
     // get a value from json object
    // or value from nested *objects* by using a key of first.second.third
    template <typename T>
    T get(std::string k, T def) const;



    // NOT WORKING YET
    template <typename T>
    bool set(std::string k, T def);

	std::string name() const
	{
		std::string n = get<std::string>("name", "YOURNAMEHERE");
		if(n == "YOURNAMEHERE")
		{
			n = boost::asio::ip::host_name();
		}
		return n;
	}
    
    std::string httpbase()
    {
        std::ostringstream s;
        s << "http://127.0.0.1"  << ":" << get<int>("http_port", DEFAULT_HTTP_PORT);
        return s.str();
    }
    
    std::string str()
    {
        std::ostringstream o;
        write_formatted( m_mainval, o );
        return o.str();
    }

    std::string filename() 
    { return m_filename; }
    
    /// gets json Value for a given key
    json_spirit::Value get_json(std::string k) const
    {
        json_spirit::Value def;
        
        std::vector<std::string> toks;
        boost::split(toks, k, boost::is_any_of("."));
        json_spirit::Value val = m_mainval;
        std::map<std::string, json_spirit::Value> mp;
        unsigned int i = 0;
        //cout << "getting: " << k << " size: " << toks.size() << endl;
        do
        {
            if(val.type() != json_spirit::obj_type) return def;
            json_spirit::obj_to_map(val.get_obj(), mp);
            if( mp.find(toks[i]) == mp.end() )
            {
                //cerr << "1 Can't find " << toks[i] << endl;
                return def;
            }
            //cout << "Got " << toks[i] << endl;
            val = mp[toks[i]];
        }
        while(++i < toks.size());
        return val;
    }

private:
   
    std::string         m_filename;
    json_spirit::Value  m_mainval;

    std::map<std::string, json_spirit::Value> m_mainmap;
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
T Config::get(std::string k, T def) const
{
    std::vector<std::string> toks;
    boost::split(toks, k, boost::is_any_of("."));
    json_spirit::Value val = m_mainval;
    std::map<std::string, json_spirit::Value> mp;
    unsigned int i = 0;
    //cout << "getting: " << k << " size: " << toks.size() << endl;
    do
    {
        if(val.type() != json_spirit::obj_type) return def;
        json_spirit::obj_to_map(val.get_obj(), mp);
        if( mp.find(toks[i]) == mp.end() )
        {
            //cerr << "1 Can't find " << toks[i] << endl;
            return def;
        }
        //cout << "Got " << toks[i] << endl;
        val = mp[toks[i]];
    }
    while(++i < toks.size());
    return val.get_value<T>();
}




template <typename T>
bool Config::set(std::string k, T def) 
{
    std::vector<std::string> toks;
    boost::split(toks, k, boost::is_any_of("."));
    json_spirit::Value * val = &m_mainval;
    json_spirit::Object o;
    std::map<std::string, json_spirit::Value> mp;
    int i = 0;
    std::cout << "getting: " << k << " size: " << toks.size() << std::endl;
    do
    {
        if(val->type() != json_spirit::obj_type)
        {
            return false; // won't replace an existing non-obj
        }
        json_spirit::obj_to_map(val->get_obj(), mp);
        if( mp.find(toks[i]) == mp.end() )
        {
            std::cerr << "Creating {} " << toks[i] << std::endl;
            mp[toks[i]] = o;
        }
        std::cout << "Got " << toks[i] << std::endl;
        val = & mp[toks[i]];
    }
    while(++i < toks.size());
    *val = json_spirit::Value(def);
    //val.get_obj().push_back( Pair(toks[i], def) );
    return true;
}

////////////////////////////////////////////////////////////////////////////////

}

#endif
