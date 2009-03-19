#ifndef __JSON_CONFIG_HPP__
#define __JSON_CONFIG_HPP__
#include <boost/asio.hpp> // for hostname.

// must be first because ossp uuid.h is stubborn and name-conflicts with
// the uuid_t in unistd.h. It gets round this with preprocessor magic. But
// this causes PAIN and HEARTACHE for everyone else in the world, so well done
// to you guys at OSSP. *claps*
#ifdef HAS_OSSP_UUID_H
#include <ossp/uuid.h>
#else
// default source package for ossp-uuid doesn't namespace itself
#include <uuid.h> 
#endif

#include "json_spirit/json_spirit.h"
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <vector>
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

	string name() const
	{
		string n = get<string>("name","YOURNAMEHERE");
		if(n=="YOURNAMEHERE")
		{
			n = boost::asio::ip::host_name();
		}
		return n;
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
    
    static string gen_uuid()
    {
        // TODO use V3 instead, so uuids are anonymous?
        char *uuid_str;
        uuid_rc_t rc;
        uuid_t *uuid;
        void *vp;
        size_t len;
    
        if((rc = uuid_create(&uuid)) != UUID_RC_OK)
        {
            uuid_destroy(uuid);
            return "";
        }
        if((rc = uuid_make(uuid, UUID_MAKE_V1)) != UUID_RC_OK)
        {
            uuid_destroy(uuid);
            return "";
        }
        len = UUID_LEN_STR+1;
        if ((vp = uuid_str = (char *)malloc(len)) == NULL) {
            uuid_destroy(uuid);
            return "";
        }
        if ((rc = uuid_export(uuid, UUID_FMT_STR, &vp, &len)) != UUID_RC_OK) {
            uuid_destroy(uuid);
            return "";
        }
        uuid_destroy(uuid);
        string retval(uuid_str);
        delete(uuid_str);
        return retval;
    }
    
    string filename() { return m_filename; }



private:


    
    string m_filename;
    Value m_mainval;
    map<string,Value> m_mainmap;
};

} //namespace

#endif
