#ifndef _PLUGIN_ADAPTOR_H_
#define _PLUGIN_ADAPTOR_H_

#include "json_spirit/json_spirit.h"
#include "playdar/types.h"
//#include "playdar/streaming_strategy.h"

namespace playdar {

class ResolverService;

class PluginAdaptor
{
protected:
    // validator and generator functions to pass to the resolver in 
    // order to generate the correct derived ResolvedItem type from json_spirit
    typedef boost::function<bool( const json_spirit::Object& )> ri_validator;
    typedef boost::function<ri_ptr( const json_spirit::Object& )> ri_generator;
    
public:
    const unsigned int api_version() const 
    { 
        // must match the version in class name
        return 1; 
    }
    
    virtual ~PluginAdaptor(){};

    virtual void               set(const std::string& key, json_spirit::Value value) = 0;
    virtual json_spirit::Value getstring(const std::string& key, const std::string& def) const = 0;
    virtual json_spirit::Value getint(const std::string& key, const int def) const = 0;
    
    template <typename T>
    inline T get(const std::string& key, const T& def) const = 0;

    // results are a vector of json result objects
    virtual bool report_results(const query_uid& qid, const std::vector< json_spirit::Object >&) = 0;

    virtual std::string gen_uuid() const = 0;
    virtual void set_rs( ResolverService * rs )
    { m_rs = rs; }

    virtual bool query_exists(const query_uid & qid) = 0;

    virtual ResolverService * rs() const { return m_rs; }
    virtual const std::string hostname() const = 0;
    
    virtual void register_resolved_item( const ri_validator&, const ri_generator& ) = 0;
    
    unsigned int targettime() const { return m_targettime; }
    unsigned short weight() const { return m_weight; }
    const bool script() const { return m_script; }
    /// TODO move to norman "get" settings API once done?:
    const std::string& scriptpath() const { return m_scriptpath; }
    
    void set_weight(unsigned short w) { m_weight = w; }
    void set_targettime(unsigned short t) { m_targettime = t; }
    void set_script(bool t) { m_script = t; }
    void set_scriptpath(std::string s) { m_scriptpath = s; }


    // TEMP!
    virtual query_uid dispatch(boost::shared_ptr<ResolverQuery> rq) = 0;
    virtual query_uid dispatch(boost::shared_ptr<ResolverQuery> rq, rq_callback_t cb) = 0;
    
    virtual ri_ptr ri_from_json( const json_spirit::Object& ) const = 0;

private:
    ResolverService * m_rs;    // instance of a plugin
    unsigned int m_targettime; // ms before passing to next resolver
    unsigned short m_weight;   // highest weight runs first.
    
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

