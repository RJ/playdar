#ifndef __MOOST_HTTP_REPLY_HPP__
#define __MOOST_HTTP_REPLY_HPP__

#include <string>
#include <vector>
#include <map>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>

#include "moost/http/header.hpp"

namespace moost { namespace http {

/// A reply to be sent to a client.
struct reply
{
  /// The status of the reply.
  enum status_type
  {
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
  } status;


  /// The content to be sent in the reply.
  std::string content;

  /// Convert the reply into a vector of buffers. The buffers do not own the
  /// underlying memory blocks, therefore the reply object must remain valid and
  /// not be changed until the write operation has completed.
  std::vector<boost::asio::const_buffer> to_buffers_headers();

  /// Get a stock reply.
  static reply stock_reply(status_type status);

  const std::vector<header>& get_headers()
  { return headers_; }

  bool has_content_fun() 
  { return content_fun_; }

   size_t read_some(char* dest, size_t size)
   {
     if ( !content_fun_ )
       return 0;
     else
       return content_fun_(dest, size);
   }

   template <typename TFun>
   void set_content_fun(const TFun& fun)
   { content_fun_ = fun; }

public:

   template <typename T>
   void add_header( const std::string& name, const T& value, bool overwrite = true )
   { add_header(name, boost::lexical_cast<std::string>(value), overwrite); }

   void add_header( const std::string& name, const std::string& value, bool overwrite );

private:

   boost::function< size_t(char*, size_t) > content_fun_;

  std::map<std::string, size_t> headersGuard_;

  /// The headers to be included in the reply.
  std::vector<header> headers_;

};

}} // moost::http

#endif // __MOOST_HTTP_REPLY_HPP__
