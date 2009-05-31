#ifndef __MOOST_HTTP_REPLY_HPP__
#define __MOOST_HTTP_REPLY_HPP__

#include <string>
#include <vector>
#include <map>
#include <boost/asio.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include "moost/http/header.hpp"

namespace moost { namespace http {

/// A reply to be sent to a client.
class reply
{
public:
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


  reply()
	:write_ending_cb_()
	,cancelled_(false)
	,writing_(false)
  {
  }

  /// Convert the reply into a vector of buffers. The buffers do not own the
  /// underlying memory blocks, therefore the reply object must remain valid and
  /// not be changed until the write operation has completed.
  std::vector<boost::asio::const_buffer> to_buffers_headers();

  /// Get a stock reply.
  void stock_reply(status_type status);

  const std::vector<header>& get_headers()
  { return headers_; }

  // The optional async delegate enables the request handler to write
  // to the client in a non-blocking manner.  The delegate is 
  // called to initiate the first write operation, and
  // subsequently after the completion of each write operation.
  // The delegate returns false to end the write sequence.
  // The WriteFunc parameter should be used once (per delegate call).
  // Keep a copy of the WriteFunc to keep the connection alive

    typedef boost::function< void(boost::asio::const_buffer&) > WriteFunc;
    typedef boost::function< bool(WriteFunc) > AsyncDelegateFunc;

public:

   template <typename T>
   void add_header( const std::string& name, const T& value, bool overwrite = true )
   { 
	   add_header(name, boost::lexical_cast<std::string>(value), overwrite); 
   }

   void add_header( const moost::http::header& h )
   { 
	   add_header(h.name, h.value, true); 
   }

   void add_header( const std::string& name, const std::string& value, bool overwrite );

   void set_status( int s )
   { 
	   status = (status_type)s; 
   }

    void write_content(const std::string& s)
	{
        boost::lock_guard<boost::mutex> lock(mutex_);
        buffers_.push_back(s);
        if (!writing_ && wf_) {
            writing_ = true;
            wf_(boost::asio::const_buffer(buffers_.front().data(), buffers_.front().length()));
        }
	}

	void write_cancel()
	{
		cancelled_ = true;
	}

	void write_finish()
	{
		write_content("");
	}

	bool async_write_delegate(WriteFunc wf)
	{
        wf_ = wf;

        if (!wf_ || cancelled_) {   
            // cancelled by caller || cancelled by us
            if (write_ending_cb_)
                write_ending_cb_();
            cancelled_ = true;
            wf_ = 0;
            return false;
        }

        {
            boost::lock_guard<boost::mutex> lock(mutex_);

            if (writing_ && buffers_.size()) {
                // previous write has completed:
                buffers_.pop_front();
                writing_ = false;
            }

            if (!writing_ && buffers_.size() && wf_) {
                // write something new
                writing_ = true;
                wf_(boost::asio::const_buffer(buffers_.front().data(), buffers_.front().length()));
            }
        }
        return true;
	}

private:

	std::map<std::string, size_t> headersGuard_;

	/// The headers to be included in the reply.
	std::vector<header> headers_;

	WriteFunc wf_;
	boost::function<void(void)> write_ending_cb_;
	bool cancelled_;
    bool writing_;

    boost::mutex mutex_;	// for protecting _buffers:
    std::list<std::string> buffers_;
};

typedef boost::shared_ptr<reply> reply_ptr;


}} // moost::http

#endif // __MOOST_HTTP_REPLY_HPP__
