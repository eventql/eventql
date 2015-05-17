/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_HTTP_HTTPREQUESTSTREAM_H
#define _FNORD_HTTP_HTTPREQUESTSTREAM_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-http/httpresponse.h>
#include <fnord-http/httpserverconnection.h>

namespace fnord {
namespace http {

class HTTPRequestStream : public RefCounted {
public:

  HTTPRequestStream(const HTTPRequest& req, HTTPServerConnection* conn);

  /**
   * Retrieve the http request. The http request will usually not contain a
   * request body
   */
  const HTTPRequest& request() const;

  /**
   * Read the http request body in chunks. This method will return once all
   * body chunks have been read
   */
  void readBody(Function<void (const void* data, size_t size)> fn);

protected:

  HTTPRequest req_;
  HTTPServerConnection* conn_;
};

}
}
#endif
