#ifndef _PLAYDAR_UTILS_BASE64_H_
#define _PLAYDAR_UTILS_BASE64_H_

#include <string>

namespace playdar { namespace utils {

std::string base64_encode(std::string const&);
std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);

}}

#endif
