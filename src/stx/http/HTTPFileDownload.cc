/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/http/HTTPFileDownload.h"

namespace stx {
namespace http {

HTTPFileDownload::HTTPFileDownload(
    const HTTPRequest& http_req,
    const String& output_file) :
    http_req_(http_req),
    output_file_(output_file),
    file_(File::openFile(output_file, File::O_CREATE | File::O_WRITE)) {}

Future<HTTPResponse> HTTPFileDownload::download(HTTPConnectionPool* http) {
  return http->executeRequest(
      http_req_,
      [this] (const Promise<HTTPResponse> promise) -> HTTPResponseFuture* {
        return new HTTPFileDownload::ResponseFuture(promise, std::move(file_));
      });
}

HTTPFileDownload::ResponseFuture::ResponseFuture(
    Promise<HTTPResponse> promise,
    File&& file) :
    HTTPResponseFuture(promise),
    file_(std::move(file)) {}

void HTTPFileDownload::ResponseFuture::onBodyChunk(
    const char* data,
    size_t size) {
  file_.write(data, size);
}

}
}
