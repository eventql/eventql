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
  String txid;

  if (!uri.getParam(params, "txid", &txid)) {
    http::HTTPResponse res;
    Buffer body;
    body.append("error: missing ?txid=... parameter");
    req_stream->readBody();
    res.populateFromRequest(req);
    res.setStatus(http::kStatusBadRequest);
    res.addBody(body.data(), body.size());
    res_stream->writeResponse(res);
    return;
  }

  http::HTTPSSEStream sse_stream(req_stream, res_stream);
  Buffer output;

  sse_stream.start();

  for (float status = 0.0; status < 100.0; status++) {
    output.clear();
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&output));
    json.beginObject();
    json.addObjectEntry("status");
    json.addString("running");
    json.addComma();
    json.addObjectEntry("progress");
    json.addFloat(status);
    json.addComma();
    json.addObjectEntry("message");
    json.addString("Running: x rows scanned");
    json.endObject();

    sse_stream.sendEvent(output, txid, "status");
    usleep(1000);
  }

  output.clear();
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&output));
  json.beginObject();
  json.addObjectEntry("result");
  json.addString("query result");
  json.endObject();

  sse_stream.sendEvent(output, txid, "result");


  sse_stream.finish();
}

}
