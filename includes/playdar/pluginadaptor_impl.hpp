#ifndef _PLUGIN_ADAPTOR_IMPL_H_
#define _PLUGIN_ADAPTOR_IMPL_H_

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

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>


#include "playdar/pluginadaptor.h"
#include "json_spirit/json_spirit.h"
#include "playdar/config.hpp"
#include "playdar/resolver.h"
#include "playdar/utils/uuid.hpp"



class PluginAdaptorImpl : public PluginAdaptor
{
public:
    PluginAdaptorImpl(Config * c, Resolver * r)
        : m_resolver(r),
          m_config(c)
    {
    }
    
    virtual void set(const std::string& key, json_spirit::Value value)
    {
        // TODO
    }
    
    virtual const std::string hostname() const
    {
        return m_config->name();
    }
    
    virtual json_spirit::Value get(const std::string& key) const
    {
        // TODO
        json_spirit::Value v( m_config->get<string>(key, "") );
        return v;
    }
    
    virtual bool report_results(const query_uid& qid, const std::vector< result_pair >& results)
    {
        std::vector< std::pair<ri_ptr,ss_ptr> > v;
        BOOST_FOREACH( const result_pair & rp, results )
        {
            ri_ptr rip = m_resolver->ri_from_json( rp.first );
            if(!rip) continue;
            v.push_back( std::pair<ri_ptr,ss_ptr>(rip, rp.second) );
        }
        m_resolver->add_results( qid, v, rs()->name() );
        return true;
    }
    
    virtual std::string gen_uuid() const
    {
        return playdar::utils::gen_uuid();
    }
    
private:
    
    Config *   m_config;
    Resolver * m_resolver;
};
#endif
