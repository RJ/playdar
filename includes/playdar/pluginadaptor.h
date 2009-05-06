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
#ifndef _PLUGIN_ADAPTOR_H_
#define _PLUGIN_ADAPTOR_H_

#include "json_spirit/json_spirit.h"
#include "playdar/types.h"
//#include "playdar/streaming_strategy.h"

namespace playdar {

class ResolverService;

class PluginAdaptor
{
public:
    const unsigned int api_version() const 
    { 
        // we'll use this if we make ABI changes later.
        return 1; 
    }
    
    virtual ~PluginAdaptor(){};

    // A short name used eg in url handler
    virtual std::string classname() const = 0;
    virtual std::string playdar_version() const  = 0;
    virtual void               set(const std::string& key, json_spirit::Value value) = 0;
    virtual json_spirit::Value getstring(const std::string& key, const std::string& def) const = 0;
    virtual json_spirit::Value getint(const std::string& key, const int def) const = 0;
    
    template <typename T>
    inline T get(const std::string& key, const T& def) const;

    // results are a vector of json result objects
    virtual bool report_results(const query_uid& qid, const std::vector< json_spirit::Object >&) = 0;

    virtual std::string gen_uuid() const = 0;
    virtual void set_rs( ResolverService * rs )
    { m_rs = rs; }

    virtual bool query_exists(const query_uid & qid) = 0;
    
    virtual std::vector< ri_ptr > get_results(query_uid qid) = 0;
    virtual int num_results(query_uid qid) = 0;
    virtual rq_ptr rq(const query_uid & qid) = 0;
    virtual void cancel_query(const query_uid & qid) = 0;
    
    virtual ResolverService * rs() const { return m_rs; }
    virtual const std::string hostname() const = 0;
    
    unsigned int targettime() const { return m_targettime; }
    unsigned short weight() const { return m_weight; }
    unsigned short preference() const { return m_preference; }
    const bool script() const { return m_script; }
    /// TODO move to normal "get" settings API once done?:
    const std::string& scriptpath() const { return m_scriptpath; }
    
    void set_weight(unsigned short w) { m_weight = w; }
    void set_preference(unsigned short p) { m_preference = p; }
    void set_targettime(unsigned short t) { m_targettime = t; }
    void set_script(bool t) { m_script = t; }
    void set_scriptpath(std::string s) { m_scriptpath = s; }


    // TEMP!
    virtual query_uid dispatch(boost::shared_ptr<ResolverQuery> rq) = 0;
    virtual query_uid dispatch(boost::shared_ptr<ResolverQuery> rq, rq_callback_t cb) = 0;

private:
    ResolverService * m_rs;    // instance of a plugin
    unsigned int m_targettime; // ms before passing to next resolver
    unsigned short m_weight;   // highest weight runs first.
    unsigned short m_preference;// secondary sort value. indicates network reliability or other user preference setting
    
    bool m_script;
    std::string m_scriptpath;
    
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <>
inline std::string PluginAdaptor::get(const std::string& k, const std::string& def) const
{
    json_spirit::Value v = getstring(k, def);
    if( v.type() != json_spirit::str_type )
        return def;

    return v.get_str();
}

template <>
inline int PluginAdaptor::get(const std::string& k, const int& def) const
{
    json_spirit::Value v = getint(k, def);
    if( v.type() != json_spirit::int_type )
        return def;
    
    return v.get_int();
}
    
////////////////////////////////////////////////////////////////////////////////

}

#endif

