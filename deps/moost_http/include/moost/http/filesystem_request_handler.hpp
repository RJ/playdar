#ifndef __MOOST_HTTP_FILESYSTEM_REQUEST_HANDLER_HPP__
#define __MOOST_HTTP_FILESYSTEM_REQUEST_HANDLER_HPP__

#include <string>
#include <boost/noncopyable.hpp>
#include "moost/http/request_handler_base.hpp"

namespace moost { namespace http {

class reply;
struct request;

/// the common handler for all incoming requests.
class filesystem_request_handler
  : public request_handler_base<filesystem_request_handler> // curiously recurring template pattern
{
public:

  filesystem_request_handler()
  : doc_root_() {}

  /// Handle a request and produce a reply.
  void handle_request(const request& req, reply& rep);

  void doc_root(const std::string & value) { doc_root_ = value; }

  std::string doc_root() { return doc_root_; }

private:

  /// The directory containing the files to be served.
  std::string doc_root_;

  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);
};

}} // moost::http

#endif // __MOOST_HTTP_FILESYSTEM_REQUEST_HANDLER_HPP__
