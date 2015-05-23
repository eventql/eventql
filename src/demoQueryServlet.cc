/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-base/inspect.h>
#include "demoQueryServlet.h"

using namespace fnord;
namespace cm {

void demoQueryServlet::handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream) {

  const http::HTTPRequest& req(req_stream->request());
  URI uri(req.uri());
  const URI::ParamList& params(uri.queryParams());

  http::HTTPSSEStream sse_stream(req_stream, res_stream);

  sse_stream.start();

  for (float status = 0.0; status < 100.0; status++) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("status");
    json.addString("running");
    json.addComma();
    json.addObjectEntry("progress");
    json.addFloat(status / 100);
    json.addComma();
    json.addObjectEntry("message");
    json.addString("Running: x rows scanned");
    json.endObject();

    sse_stream.sendEvent(buf, Some(String("status")));
    usleep(100000);
  }

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  json.beginObject();
  json.addObjectEntry("result");
  json.addString("query result");
  json.endObject();

  sse_stream.sendEvent(buf, Some(String("result")));
  sse_stream.finish();
}

}
