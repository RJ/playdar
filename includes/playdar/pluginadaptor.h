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
    typedef std::pair< json_spirit::Object, ss_ptr > result_pair;
    
public:
    const unsigned int api_version() const 
    { 
        // must match the version in class name
        return 1; 
    }
    
    virtual ~PluginAdaptor(){};
    virtual void set(const std::string& key, json_spirit::Value value) = 0;

    virtual json_spirit::Value get(const std::string& key) const = 0;
    virtual bool report_results(const query_uid& qid, const std::vector< result_pair >&) = 0;

    virtual std::string gen_uuid() const = 0;
    virtual void set_rs( ResolverService * rs )
    {
        m_rs = rs;
    }
    virtual ResolverService * rs() const { return m_rs; }
    virtual const std::string hostname() const = 0;
    
    unsigned int targettime() const { return m_targettime; }
    unsigned short weight() const { return m_weight; }
    const bool script() const { return m_script; }
    
    void set_weight(unsigned short w) { m_weight = w; }
    void set_targettime(unsigned short t) { m_targettime = t; }
    void set_script(bool t) { m_script = t; }

private:
    ResolverService * m_rs;    // instance of a plugin
    unsigned int m_targettime; // ms before passing to next resolver
    unsigned short m_weight;   // highest weight runs first.
    bool m_script;
    
};

}

#endif

