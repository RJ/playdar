#ifndef LAME_UUIDS_PLEASE
#include <ossp/uuid.h> // the cpp version of this isn't in mac ports
#endif
#include "application/application.h"
#include "library/library.h"
#include "resolvers/resolver.h"
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
//#include <cstdlib.h>

using namespace std;

MyApplication::MyApplication(boost::program_options::variables_map opt)
{
    
    m_po = opt;
    string db_path = option<string>("app.db");
    m_library   = new Library( db_path, this );
    m_resolver  = new Resolver(this);
    
    m_ios = boost::shared_ptr<boost::asio::io_service>
                                    (new boost::asio::io_service);
    m_work = boost::shared_ptr<boost::asio::io_service::work>
                (new boost::asio::io_service::work(*m_ios));
    
    do_auto_config();
}

MyApplication::~MyApplication()
{
    delete(m_library);
}

void
MyApplication::do_auto_config()
{
    cout << "Autodetecting stuff..." << endl;
    string hostname = boost::asio::ip::host_name();
    cout << "Hostname: " << hostname << endl;
    boost::asio::ip::tcp::resolver resolver(*m_ios);
    boost::asio::ip::tcp::resolver::query 
        query(hostname, "");
    boost::asio::ip::tcp::resolver::iterator iter =
        resolver.resolve(query);
    boost::asio::ip::tcp::resolver::iterator end_marker;
    boost::asio::ip::tcp::endpoint ep;
    boost::asio::ip::address_v4 ipaddr;
    while (iter != end_marker)
    { 
        ep = *iter++;
        cout << "Found address: " << ep.address().to_string() << endl;
    }
    /*
    ep.port(m_port);
        }
        m_socket = boost::shared_ptr<boost::asio::ip::tcp::socket>(new tcp::socket(m_io_service));

        boost::system::error_code error = boost::asio::error::host_not_found;
        m_socket->connect(ep, error);
        if (error) throw boost::system::system_error(error);
*/

    

}

string 
MyApplication::gen_uuid()
{
#ifndef LAME_UUIDS_PLEASE
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
#else
    // "uuid" hack based on random numbers. not good.
    ostringstream s;
    s << "lame_uuid_"
      << "_"
      << rand();
    return s.str();
#endif
}



template <typename T> T
MyApplication::option(string o, T def)
{
    if(m_po.count(o)==0)
    {
        cerr << "Option '"<<  o <<"' not set, using default: " << def << endl;
        return def;
    }
    return m_po[o].as<T>();
}

template <typename T> T
MyApplication::option(string o)
{
    if(m_po.count(o)==0)
    {
        cerr << "WARNING: Option '"<<  o <<"' not set! creating empty value" << endl;
        T def;
        return def; 
    }
    return m_po[o].as<T>();
}


std::string 
MyApplication::name()
{ 
    return option<string>("app.name"); 
}

unsigned short 
MyApplication::http_port() 
{ 
    return (unsigned short)(option<int>("app.http_port")); 
}

unsigned short 
MyApplication::multicast_port() 
{ 
    return (unsigned short)(option<int>("resolver.lan_udp.port"));
}

boost::asio::ip::address_v4 
MyApplication::private_ip()
{
    //return boost::asio::ip::address_v4::from_string( option<string>("app.private_ip") );
    return boost::asio::ip::address_v4::from_string("127.0.0.1");
}

boost::asio::ip::address_v4 
MyApplication::public_ip()
{
    return private_ip();
}

boost::asio::ip::address_v4 
MyApplication::multicast_ip()
{
    return boost::asio::ip::address_v4::from_string( option<string>("resolver.lan_udp.multicast") );
}

// get base http url on private network, no trailing slash
string 
MyApplication::httpbase()
{
    ostringstream s;
    s << "http://" << private_ip().to_string() << ":" << http_port();
    return s.str();
}
    
Library * 
MyApplication::library()
{ 
    return m_library; 
}

Resolver *
MyApplication::resolver()
{
    return m_resolver;
}


int 
MyApplication::levenshtein(const std::string & source, const std::string & target) {

  // Step 1

  const int n = source.length();
  const int m = target.length();
  if (n == 0) {
    return m;
  }
  if (m == 0) {
    return n;
  }

  // Good form to declare a TYPEDEF

  typedef std::vector< std::vector<int> > Tmatrix; 

  Tmatrix matrix(n+1);

  // Size the vectors in the 2.nd dimension. Unfortunately C++ doesn't
  // allow for allocation on declaration of 2.nd dimension of vec of vec

  for (int i = 0; i <= n; i++) {
    matrix[i].resize(m+1);
  }

  // Step 2

  for (int i = 0; i <= n; i++) {
    matrix[i][0]=i;
  }

  for (int j = 0; j <= m; j++) {
    matrix[0][j]=j;
  }

  // Step 3

  for (int i = 1; i <= n; i++) {

    const char s_i = source[i-1];

    // Step 4

    for (int j = 1; j <= m; j++) {

      const char t_j = target[j-1];

      // Step 5

      int cost;
      if (s_i == t_j) {
        cost = 0;
      }
      else {
        cost = 1;
      }

      // Step 6

      const int above = matrix[i-1][j];
      const int left = matrix[i][j-1];
      const int diag = matrix[i-1][j-1];
      int cell = min( above + 1, min(left + 1, diag + cost));

      // Step 6A: Cover transposition, in addition to deletion,
      // insertion and substitution. This step is taken from:
      // Berghel, Hal ; Roach, David : "An Extension of Ukkonen's 
      // Enhanced Dynamic Programming ASM Algorithm"
      // (http://www.acm.org/~hlb/publications/asm/asm.html)

      if (i>2 && j>2) {
        int trans=matrix[i-2][j-2]+1;
        if (source[i-2]!=t_j) trans++;
        if (s_i!=target[j-2]) trans++;
        if (cell>trans) cell=trans;
      }

      matrix[i][j]=cell;
    }
  }

  // Step 7

  return matrix[n][m];
}




