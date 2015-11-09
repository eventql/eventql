/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_HTTP_HTTPREQUESTSTREAM_H
#define _STX_HTTP_HTTPREQUESTSTREAM_H
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <stx/http/httpresponse.h>
#include <stx/http/httpserverconnection.h>

namespace stx {
namespace http {

class HTTPRequestStream : public RefCounted {
public:

  HTTPRequestStream(const HTTPRequest& req, HTTPServerConnection* conn);

  /**
   * Retrieve the http request. The http request will not contain a request body
   * by default. Instead you must call one of the readBody(...) method to
   * retrieve the request body
   */
  const HTTPRequest& request() const;

  /**
   * Read the http request body in chunks. This method will call the provided
   * callback for each chunk and return once all body chunks have been read
   */
  void readBody(Function<void (const void* data, size_t size)> fn);

  /**
   * Read the http request body into the contained HTTPRequest
   */
  void readBody();

  /**
   * Discard the http request body
   */
  void discardBody();

protected:
  HTTPRequest req_;
  HTTPServerConnection* conn_;
};

}
}
#endif
