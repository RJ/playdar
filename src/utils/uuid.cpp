#include "playdar/utils/uuid.h"

#include <sstream>
#include <boost/uuid.hpp>

namespace playdar {
namespace utils { 

class uuid_pimpl
{
public:
    std::string gen()
    {
        boost::uuids::uuid u = m_gen();
        std::ostringstream oss;
        oss << u;
        return oss.str();
    }
    boost::uuids::random_generator<> m_gen;
};

// use it like:
// uuid_gen gen;
// string val = gen();

uuid_gen::uuid_gen()
    : m_pimpl( new uuid_pimpl() )
{}
    
std::string uuid_gen::operator()()
{
     return m_pimpl->gen();
}



}} // ns

