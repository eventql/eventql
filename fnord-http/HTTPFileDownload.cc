/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-http/HTTPFileDownload.h"

namespace fnord {
namespace http {

HTTPFileDownload::HTTPFileDownload(
    const HTTPRequest& http_req,
    const String& output_file) :
    http_req_(http_req),
    output_file_(output_file) {}

Future<HTTPResponse> HTTPFileDownload::download(HTTPConnectionPool* http) {
  return http->executeRequest(
      http_req_,
      [] (const Promise<HTTPResponse> promise) {
        return new HTTPResponseFuture(promise);
      });
}

}
}
