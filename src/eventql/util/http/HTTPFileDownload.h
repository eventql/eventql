/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_HTTP_FILEDOWNLOAD_H
#define _STX_HTTP_FILEDOWNLOAD_H
#include <eventql/util/uri.h>
#include <eventql/util/io/file.h>
#include <eventql/util/http/httpmessage.h>
#include "eventql/util/http/httprequest.h"
#include "eventql/util/http/httpresponse.h"
#include "eventql/util/http/httpstats.h"
#include "eventql/util/http/httpconnectionpool.h"
#include "eventql/util/http/httpclient.h"
#include <string>

namespace stx {
namespace http {

class HTTPFileDownload {
public:

  HTTPFileDownload(
      const HTTPRequest& http_req,
      const String& output_file_);

  Future<HTTPResponse> download(HTTPConnectionPool* http);
  HTTPResponse download(HTTPClient* http);

protected:
  class ResponseFuture : public HTTPResponseFuture {
  public:
    ResponseFuture(Promise<HTTPResponse> promise, File&& file);
    void onBodyChunk(const char* data, size_t size) override;
    File file_;
  };

  HTTPRequest http_req_;
  String output_file_;
  File file_;
};

}
}
#endif
