/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _STX_HTTP_SERVERCONNECTION_H
#define _STX_HTTP_SERVERCONNECTION_H
#include <memory>
#include <vector>
#include <eventql/util/autoref.h>
#include <eventql/util/stdtypes.h>
#include <eventql/util/http/httphandler.h>
#include <eventql/util/http/httpparser.h>
#include <eventql/util/http/httprequest.h>
#include <eventql/util/http/httpresponse.h>
#include <eventql/util/http/httpstats.h>
#include <eventql/util/net/tcpconnection.h>
#include <eventql/util/thread/taskscheduler.h>
#include <eventql/util/return_code.h>

namespace http {

class HTTPServerConnection {
public:

  HTTPServerConnection(
      int fd,
      uint64_t io_timeout,
      const std::string& prelude_bytes);

  ~HTTPServerConnection();

  ReturnCode recvRequest(HTTPRequest* request);
  ReturnCode sendResponse(const HTTPResponse& res);
  ReturnCode sendResponseHeaders(const HTTPResponse& res);
  ReturnCode sendResponseBodyChunk(const char* data, size_t size);

  bool isClosed() const;
  void close();

protected:

  ReturnCode write(const char* data, size_t size);

  int fd_;
  uint64_t io_timeout_;
  std::string prelude_bytes_;
  bool closed_;
};

}
#endif
