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
#ifndef _PLUGIN_ADAPTOR_IMPL_H_
#define _PLUGIN_ADAPTOR_IMPL_H_

//// must be first because ossp uuid.h is stubborn and name-conflicts with
//// the uuid_t in unistd.h. It gets round this with preprocessor magic. But
//// this causes PAIN and HEARTACHE for everyone else in the world, so well done
//// to you guys at OSSP. *claps*
//#ifdef HAS_OSSP_UUID_H
//#include <ossp/uuid.h>
//#else
//// default source package for ossp-uuid doesn't namespace itself
//#include <uuid.h> 
//#endif

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>

#include "playdar/pluginadaptor.h"
#include "json_spirit/json_spirit.h"
#include "playdar/config.hpp"
#include "playdar/resolver.h"

namespace playdar {

class PluginAdaptorImpl : public PluginAdaptor
{
public:
    PluginAdaptorImpl(Config * c, Resolver * r, const std::string& className)
        : m_config(c), 
          m_resolver(r),
          m_className( className )
    {
    }
    
    virtual std::string playdar_version() const 
    {
        return VERSION;
    }
    
    virtual void set(const std::string& key, json_spirit::Value value)
    {
        // TODO
    }
    
    // A short name used eg in url handler
    virtual std::string classname() const
    {
        return m_className;
    }

    virtual const std::string hostname() const
    {
        return m_config->name();
    }
    
    virtual json_spirit::Value getstring(const std::string& key, const std::string& def) const
    {
        // TODO
        json_spirit::Value v( m_config->get<std::string>(key, def) );
        return v;
    }
    
    virtual json_spirit::Value getint(const std::string& key, const int def) const
    {
        // TODO
        json_spirit::Value v( m_config->get<int>(key, def) );
        return v;
    }
    
    virtual bool report_results(const query_uid& qid, const std::vector< json_spirit::Object >& results)
    {
        std::vector< ri_ptr > v;
        BOOST_FOREACH( const json_spirit::Object & o, results )
        {
            ri_ptr rip(new ResolvedItem( o ));
            // if no preference specified, set to preference of this plugin.
            // in some cases, plugins will know varied preferences per-result
            // eg: a p2p plugin would know certain peers are less reliable
            // but normally you just use a global value per plugin.
            // ie Local > LAN > some web service
            if( rip->preference() == -1 )
            {
                // not set, use global pref of this plugin:
                rip->set_preference( preference() );
            }
            else
            {
                // use lowest preference of reported, or our own for this plugin
                if( preference() < rip->preference() )
                    rip->set_preference( preference() );
            }
            v.push_back( rip );
        }
        m_resolver->add_results( qid, v, rs()->name() );
        return true;
    }
    
    virtual std::string gen_uuid() const
    {
        return m_resolver->gen_uuid();
    }

    virtual bool query_exists(const query_uid & qid)
    {
        return m_resolver->query_exists(qid); 
    }
    
    virtual std::vector< ri_ptr > get_results(query_uid qid)
    {
        return m_resolver->get_results(qid); 
    }
    
    virtual rq_ptr rq(const query_uid & qid)
    {
        return m_resolver->rq(qid); 
    }
    
    virtual int num_results(query_uid qid)
    {
        return m_resolver->num_results(qid); 
    }
    
    virtual void cancel_query(const query_uid & qid)
    {
        m_resolver->cancel_query(qid); 
    }

    virtual query_uid dispatch(boost::shared_ptr<ResolverQuery> rq)
    { 
        return m_resolver->dispatch(rq); 
    }
    
    virtual query_uid dispatch(boost::shared_ptr<ResolverQuery> rq, rq_callback_t cb)
    { 
        return m_resolver->dispatch(rq, cb); 
    }


private:
    Config*   m_config;
    Resolver* m_resolver;
    std::string m_className;
};

} // ns

#endif
