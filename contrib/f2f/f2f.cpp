#include "f2f.h"

#include "playdar/resolver_query.hpp"
#include "playdar/playdar_request.h"

using namespace std;
using namespace json_spirit;

namespace playdar {
namespace resolvers {

bool
f2f::init(pa_ptr pap)
{
    m_pap = pap;
    string jid = m_pap->get<string>("plugins.f2f.jid","");
    string pass= m_pap->get<string>("plugins.f2f.pass","");
    if( jid.empty() || pass.empty() )
    {
        cerr << "f2f plugin not initializing - no jid+pass in config file." << endl;
        return false;
    }
    m_jbot = boost::shared_ptr<jbot>(new jbot(jid, pass));
    m_jbot_thread = boost::shared_ptr<boost::thread>
                        (new boost::thread(boost::bind(&jbot::start, m_jbot)));
    return true;
}



f2f::~f2f() throw()
{
    m_jbot->stop();
    m_jbot_thread->join();
}

void
f2f::start_resolving(boost::shared_ptr<ResolverQuery> rq)
{
    using namespace json_spirit;
    ostringstream querystr;
    write_formatted( rq->get_json(), querystr );
    cout << "Resolving: " << querystr.str() << " through the f2f plugin" << endl;
}

void 
f2f::cancel_query(query_uid qid)
{}

bool
f2f::anon_http_handler(const playdar_request& req, playdar_response& resp)
{
    cout << "request handler on f2f for url: " << req.url() << endl;
    ostringstream os;
    os <<   "<h2>f2f info</h2>"
            "todo"
            ;
    resp = playdar_response(os.str(), true);
    return true;
}

}} // namespaces
