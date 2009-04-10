#ifndef __JSON_CONFIG_HPP__
#define __JSON_CONFIG_HPP__

#include <map>
#include <vector>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "json_spirit/json_spirit.h"

namespace playdar {

// wrapper around JSON config file
// instance of this is also passed to plugins.
class Config
{

public:
    Config(std::string f);
    void reload();

    std::string name() const;
    std::string httpbase();
    std::string str();

    std::string filename() { return m_filename; }

    static std::string gen_uuid();

    // get a value from json object
    // or value from nested *objects* by using a key of first.second.third
    template <typename T>
    T get(std::string k, T def) const;

    // NOT WORKING YET
    template <typename T>
    bool set(std::string k, T def);

private:
    
    std::string          m_filename;
    json_spirit::Value   m_mainval;
    std::map<std::string, json_spirit::Value> m_mainmap;
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
T Config::get(std::string k, T def) const
{
    std::vector<std::string> toks;
    boost::split(toks, k, boost::is_any_of("."));
    json_spirit::Value val = m_mainval;
    std::map<string, json_spirit::Value> mp;
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
    cout << "getting: " << k << " size: " << toks.size() << endl;
    do
    {
        if(val->type() != obj_type)
        {
            return false; // won't replace an existing non-obj
        }
        json_spirit::obj_to_map(val->get_obj(), mp);
        if( mp.find(toks[i]) == mp.end() )
        {
            cerr << "Creating {} " << toks[i] << endl;
            mp[toks[i]] = o;
        }
        cout << "Got " << toks[i] << endl;
        val = & mp[toks[i]];
    }
    while(++i < toks.size());
    *val = json_spirit::Value(def);
    //val.get_obj().push_back( Pair(toks[i], def) );
    return true;
}

////////////////////////////////////////////////////////////////////////////////

} //namespace

#endif
