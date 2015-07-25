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
#include <stx/uri.h>
#include <stx/io/file.h>
#include <stx/http/httpmessage.h>
#include "stx/http/httprequest.h"
#include "stx/http/httpresponse.h"
#include "stx/http/httpstats.h"
#include "stx/http/httpconnectionpool.h"
#include <string>

namespace stx {
namespace http {

class HTTPFileDownload {
public:

  HTTPFileDownload(
      const HTTPRequest& http_req,
      const String& output_file_);

  Future<HTTPResponse> download(HTTPConnectionPool* http);

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
