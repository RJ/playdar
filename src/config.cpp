#include "playdar/config.h"

#include <fstream>
#include <sstream>

#include <boost/asio.hpp> // for hostname
#include <boost/uuid.hpp>

using namespace std;
using namespace json_spirit;

namespace playdar {

Config::Config(string f) 
: m_filename(f)
{
    reload();
}

void Config::reload()
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

string Config::name() const
{
	string n = get<string>("name","YOURNAMEHERE");
	if( n == "YOURNAMEHERE" )
	{
		n = boost::asio::ip::host_name();
	}
	return n;
}

string Config::httpbase()
{
    ostringstream s;
    s << "http://127.0.0.1"  << ":" << get<int>("http_port", 8888);
    return s.str();
}


string Config::str()
{
    ostringstream o;
    write_formatted( m_mainval, o );
    return o.str();
}
    
string Config::gen_uuid()
{
//    // TODO use V3 instead, so uuids are anonymous?
//    char *uuid_str;
//    uuid_rc_t rc;
//    uuid_t *uuid;
//    void *vp;
//    size_t len;
//
//    if((rc = uuid_create(&uuid)) != UUID_RC_OK)
//    {
//        uuid_destroy(uuid);
//        return "";
//    }
//    if((rc = uuid_make(uuid, UUID_MAKE_V1)) != UUID_RC_OK)
//    {
//        uuid_destroy(uuid);
//        return "";
//    }
//    len = UUID_LEN_STR+1;
//    if ((vp = uuid_str = (char *)malloc(len)) == NULL) {
//        uuid_destroy(uuid);
//        return "";
//    }
//    if ((rc = uuid_export(uuid, UUID_FMT_STR, &vp, &len)) != UUID_RC_OK) {
//        uuid_destroy(uuid);
//        return "";
//    }
//    uuid_destroy(uuid);
//    string retval(uuid_str);
//    delete(uuid_str);
//    return retval;

   boost::uuids::random_generator<> gen1;
   boost::uuids::uuid u = gen1();

   ostringstream oss;
   oss << boost::uuids::showbraces << u;
   // output "{00000000-0000-0000-0000-000000000000}"

   return oss.str();
}

}
