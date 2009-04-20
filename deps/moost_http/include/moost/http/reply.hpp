#ifndef __MOOST_HTTP_REPLY_HPP__
#define __MOOST_HTTP_REPLY_HPP__

#include "playdar/application.h" // if not first mac compile fails
#include "playdar/streaming_strategy.h"

#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

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

  /// The headers to be included in the reply.
  std::vector<header> headers;

  /// The content to be sent in the reply.
  std::string content;

  /// Convert the reply into a vector of buffers. The buffers do not own the
  /// underlying memory blocks, therefore the reply object must remain valid and
  /// not be changed until the write operation has completed.
  std::vector<boost::asio::const_buffer> to_buffers(bool inc_body = true);

  /// Get a stock reply.
  static reply stock_reply(status_type status);
  
  // only needed if we stream response:
  
  /// true if handler will stream body after headers sent
  /// false means entire body prepared up-front.
  bool m_streaming;
  size_t m_streaming_len;
  boost::shared_ptr<playdar::StreamingStrategy> m_ss;
  
  void set_streaming(boost::shared_ptr<playdar::StreamingStrategy> ss, 
                     size_t len)
  { 
    m_streaming=true; 
    m_streaming_len = len;
    m_ss = ss;
  }
  
  void unset_streaming()
  {
    m_streaming = false;
  }
  
  // get streaming strategy, for streaming response
  boost::shared_ptr<playdar::StreamingStrategy> get_ss()
  {
    return m_ss;
  }
  
  size_t streaming_length() { return m_streaming_len; }
  bool streaming() { return m_streaming; }
  
};

}} // moost::http

#endif // __MOOST_HTTP_REPLY_HPP__
