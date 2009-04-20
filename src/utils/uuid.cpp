#include "playdar/utils/uuid.h"

#include <sstream>
#include <boost/uuid.hpp>

namespace playdar {
namespace utils { 

// use it like:
// uuid_gen gen;
// string val = gen();

class uuid_gen
{
public:

   std::string operator()()
   {
      boost::uuids::uuid u = m_gen();

      std::ostringstream oss;
      oss << boost::uuids::showbraces << u;
      return oss.str();
   }

private:
    boost::uuids::random_generator<> m_gen;
};


}} // ns

