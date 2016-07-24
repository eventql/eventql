/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include "WriteStreamServlet.h"

namespace http {

void WriteStreamServlet::handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream) {


  HTTPResponse resp;
  Buffer chunk;

  chunk.append("Ping Pong\n");
  req_stream->readBody();
  resp.populateFromRequest(req_stream->request());
  resp.setStatus(kStatusOK);
  res_stream->startResponse(resp);

  for (int i = 0; i < 100; ++i) {
    res_stream->writeBodyChunk(chunk);
    usleep(100000);
  }

  res_stream->finishResponse();
}

}

