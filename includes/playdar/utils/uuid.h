#ifndef _PLAYDAR_UTILS_UUID_H_
#define _PLAYDAR_UTILS_UUID_H_

#include <string>
#include <boost/shared_ptr.hpp>

namespace playdar {
namespace utils { 

class uuid_pimpl;

class uuid_gen
{
public:
    uuid_gen();
    std::string operator()();
private:
    boost::shared_ptr<uuid_pimpl> m_pimpl;
};


}} // ns

#endif
