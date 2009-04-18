#include "playdar/utils/uuid.h"

#include <sstream>
#include <boost/uuid.hpp>

namespace playdar {
namespace utils { 

    std::string gen_uuid()
    {
       boost::uuids::random_generator<> gen1;
       boost::uuids::uuid u = gen1();

       std::ostringstream oss;
       oss << boost::uuids::showbraces << u;
       // output "{00000000-0000-0000-0000-000000000000}"

       return oss.str();
    }

}} // ns

