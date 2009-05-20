#ifndef __MOOST_HTTP_REQUEST_HANDLER_BASE_HPP__
#define __MOOST_HTTP_REQUEST_HANDLER_BASE_HPP__

#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>

#include "moost/http/reply.hpp"
#include "moost/http/request.hpp"

namespace moost { namespace http {

/// the common handler for all incoming requests.
template<class RequestHandler>
struct request_handler_base
  : private boost::noncopyable
{
  /// handle a request, configure set up headers, pass on to handle_request
  void handle_request_base(const request& req, reply& rep)
  {
    rep.status = reply::ok;
    static_cast< RequestHandler * >(this)->handle_request(req, rep);
    if( !rep.get_async_delegate() )
    {
        rep.add_header( "Content-Length",  boost::lexical_cast<std::string>(rep.content.length()) );
    }
    rep.add_header( "Content-Type", "text/plain", false );
  }

  void handle_request(const request& req, reply& rep)
  {
    // default base implementation does nothing
  }
};

}} // moost::http

#endif // __MOOST_HTTP_REQUEST_HANDLER_BASE_HPP__
