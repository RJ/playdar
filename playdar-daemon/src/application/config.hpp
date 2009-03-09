#ifndef __JSON_CONFIG_HPP__
#define __JSON_CONFIG_HPP__

#include "json_spirit/json_spirit.h"
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <boost/algorithm/string.hpp>

namespace playdar {

using namespace std;
using namespace json_spirit;

// wrapper around JSON config file
// instance of this is also passed to plugins.
class Config
{

public:
    Config(string f)
        : m_filename(f)
    {

        reload();
    }
    
    void reload()
    {
        fstream ifs;
        ifs.open(m_filename.c_str(), ifstream::in);
        if(ifs.fail())
        {
            cerr << "Failed to open config file: " << m_filename << endl;
            throw;
        }
        if(!read(ifs, m_mainval))
        {
            cerr << "Failed to parse config file: " << m_filename << endl;
            throw;
        }
        if(m_mainval.type() != obj_type)
        {
            cerr << "Config file isn't a JSON object!" << endl;
            throw;
        }
        obj_to_map(m_mainval.get_obj(), m_mainmap);
    }
    
    // get a value from json object
    // or value from nested *objects* by using a key of first.second.third
    template <typename T>
    T get(string k) const
    {
        T def;
        return get<T>(k,def); 
    }
    
    template <typename T>
    T get(string k, T def) const
    {
        std::vector<std::string> toks;
        boost::split(toks, k, boost::is_any_of("."));
        Value val = m_mainval;
        map<string,Value> mp;
        unsigned int i = 0;
        //cout << "getting: " << k << " size: " << toks.size() << endl;
        do
        {
            if(val.type() != obj_type) return def;
            obj_to_map(val.get_obj(), mp);
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
    
    string httpbase()
    {
        ostringstream s;
        s << "http://127.0.0.1"  << ":" << get<int>("http_port", 8888);
        return s.str();
    }

    // NOT WORKING YET
    template <typename T>
    bool set(string k, T def) 
    {
        std::vector<std::string> toks;
        boost::split(toks, k, boost::is_any_of("."));
        Value * val = &m_mainval;
        Object o;
        map<string,Value> mp;
        int i = 0;
        cout << "getting: " << k << " size: " << toks.size() << endl;
        do
        {
            if(val->type() != obj_type)
            {
                return false; // won't replace an existing non-obj
            }
            obj_to_map(val->get_obj(), mp);
            if( mp.find(toks[i]) == mp.end() )
            {
                cerr << "Creating {} " << toks[i] << endl;
                mp[toks[i]] = o;
            }
            cout << "Got " << toks[i] << endl;
            val = & mp[toks[i]];
        }
        while(++i < toks.size());
        *val = Value(def);
        //val.get_obj().push_back( Pair(toks[i], def) );
        return true;
    }
    
    string str()
    {
        ostringstream o;
        write_formatted( m_mainval, o );
        return o.str();
    }

private:
    string m_filename;
    Value m_mainval;
    map<string,Value> m_mainmap;
};

} //namespace

#endif
