/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/util/wallclock.h"
#include "eventql/util/assets.h"
#include <eventql/util/fnv.h>
#include "eventql/util/protobuf/msg.h"
#include "eventql/metrics/MetricAPIServlet.h"
#include "eventql/metrics/MetricQuery.h"

using namespace stx;

namespace zbase {

MetricAPIServlet::MetricAPIServlet(
    MetricService* service,
    ConfigDirectory* cdir,
    const String& cachedir) :
    service_(service),
    cdir_(cdir),
    cachedir_(cachedir) {}

void MetricAPIServlet::handle(
    const AnalyticsSession& session,
    RefPtr<stx::http::HTTPRequestStream> req_stream,
    RefPtr<stx::http::HTTPResponseStream> res_stream) {
  const auto& req = req_stream->request();
  URI uri(req.uri());

  http::HTTPResponse res;
  res.populateFromRequest(req);

  if (uri.path() == "/api/v1/metrics/query") {
    executeQuery(session, uri, req_stream.get(), res_stream.get());
    return;
  }

  res.setStatus(http::kStatusNotFound);
  res.addHeader("Content-Type", "text/html; charset=utf-8");
  res.addBody(Assets::getAsset("eventql/webui/404.html"));
  res_stream->writeResponse(res);
}

void MetricAPIServlet::executeQuery(
    const AnalyticsSession& session,
    const URI& uri,
    http::HTTPRequestStream* req_stream,
    http::HTTPResponseStream* res_stream) {
  req_stream->readBody();

  auto sse_stream = mkRef(new http::HTTPSSEStream(req_stream, res_stream));
  sse_stream->start();

  auto send_progress = [this, sse_stream] (double progress) {
    if (sse_stream->isClosed()) {
      return;
    }

    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("status");
    json.addString("running");
    json.addComma();
    json.addObjectEntry("progress");
    json.addFloat(progress);
    json.endObject();
    sse_stream->sendEvent(buf, Some(String("status")));
  };

  try {
    auto query = mkRef(new MetricQuery());
    query->onProgress(send_progress);

    auto jq = json::parseJSON(req_stream->request().body());
    auto jmetrics = json::objectLookup(jq, "metrics");
    if (jmetrics == jq.end()) {
      RAISE(kRuntimeError, "missing field: metrics");
    }

    for (auto cur = jmetrics + 1; cur < jmetrics + jmetrics->size - 1; ) {
      MetricDefinition metric;
      metric.set_metric_key(cur->data);
      ++cur;

      auto metric_source = json::objectGetString(cur, jq.end(), "source");
      if (!metric_source.isEmpty()) {
        metric.set_source(metric_source.get());
      }

      auto metric_expr = json::objectGetString(cur, jq.end(), "expr");
      if (!metric_expr.isEmpty()) {
        metric.set_expr(metric_expr.get());
      }

      auto metric_filter = json::objectGetString(cur, jq.end(), "filter");
      if (!metric_filter.isEmpty()) {
        metric.set_filter(metric_filter.get());
      }

      cur += cur->size;
    }

    send_progress(0);
    service_->executeQuery(session, query);

    {
      Buffer buf;
      json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
      json.beginObject();
      json.endObject();
      sse_stream->sendEvent(buf, Some(String("result")));
    }
  } catch (const StandardException& e) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("status");
    json.addString("error");
    json.addComma();
    json.addObjectEntry("error");
    json.addString(e.what());
    json.endObject();

    sse_stream->sendEvent(buf, Some(String("status")));
    sse_stream->sendEvent(URI::urlEncode(e.what()), Some(String("error")));
  }

  sse_stream->finish();
}

}
