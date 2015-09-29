/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "stx/assets.h"
#include "stx/protobuf/msg.h"
#include "stx/io/BufferedOutputStream.h"
#include "zbase/api/EventsAPIServlet.h"

using namespace stx;

namespace zbase {

EventsAPIServlet::EventsAPIServlet(
    EventsService* service,
    ConfigDirectory* cdir,
    const String& cachedir) :
    service_(service),
    cdir_(cdir),
    cachedir_(cachedir) {}

void EventsAPIServlet::handle(
    const AnalyticsSession& session,
    RefPtr<stx::http::HTTPRequestStream> req_stream,
    RefPtr<stx::http::HTTPResponseStream> res_stream) {
  const auto& req = req_stream->request();
  URI uri(req.uri());

  http::HTTPResponse res;
  res.populateFromRequest(req);

  if (uri.path() == "/api/v1/events/scan") {
    scanTable(session, uri, req_stream.get(), res_stream.get());
    return;
  }

  if (uri.path() == "/api/v1/events/scan_partition") {
    scanTablePartition(session, uri, req_stream.get(), res_stream.get());
    return;
  }

  res.setStatus(http::kStatusNotFound);
  res.addHeader("Content-Type", "text/html; charset=utf-8");
  res.addBody(Assets::getAsset("zbase/webui/404.html"));
  res_stream->writeResponse(res);
}

void EventsAPIServlet::scanTable(
    const AnalyticsSession& session,
    const URI& uri,
    http::HTTPRequestStream* req_stream,
    http::HTTPResponseStream* res_stream) {
  req_stream->readBody();

  const auto& params = uri.queryParams();

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    http::HTTPResponse res;
    res.populateFromRequest(req_stream->request());
    res.setStatus(http::kStatusBadRequest);
    res.addBody("error: missing ?table=... parameter");
    res_stream->writeResponse(res);
    return;
  }

  EventScanParams scan_params;

  scan_params.set_end_time(WallClock::unixMicros());

  String time_str;
  if (URI::getParam(params, "time", &time_str)) {
    scan_params.set_end_time(std::stoull(time_str));
  }

  size_t limit = 1000;
  String limit_str;
  if (URI::getParam(params, "limit", &limit_str)) {
    limit = std::stoull(limit_str);
  }

  String columns_str;
  if (URI::getParam(params, "columns", &columns_str)) {
    for (const auto& c : StringUtil::split(columns_str, ",")) {
      *scan_params.add_columns() = c;
    }
  }

  EventScanResult result(limit);

  http::HTTPSSEStream sse_stream(req_stream, res_stream);
  sse_stream.start();

  auto send_status_update = [&sse_stream, &result, &scan_params] (bool done) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("status");
    json.addString(done ? "finished" : "running");
    json.addComma();
    json.addObjectEntry("scanned_until");
    json.addInteger(result.scannedUntil().unixMicros());
    json.addComma();
    json.addObjectEntry("rows_scanned");
    json.addInteger(result.rowScanned());
    json.addComma();
    json.addObjectEntry("result");
    json.beginArray();

    size_t nline = 0;
    for (const auto& l : result.rows()) {
      if (++nline > 1) {
        json.addComma();
      }

      json.beginObject();
      json.addObjectEntry("time");
      json.addInteger(l.time.unixMicros());
      json.addComma();

      json.addObjectEntry("time");
      l.obj.toJSON(&json);

      json.endObject();
    }

    json.endArray();
    json.endObject();

    sse_stream.sendEvent(buf, None<String>());
  };

  service_->scanTable(
      session,
      table_name,
      scan_params,
      &result,
      send_status_update);

  sse_stream.finish();
}

void EventsAPIServlet::scanTablePartition(
    const AnalyticsSession& session,
    const URI& uri,
    http::HTTPRequestStream* req_stream,
    http::HTTPResponseStream* res_stream) {
  req_stream->readBody();

  const auto& params = uri.queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    http::HTTPResponse res;
    res.populateFromRequest(req_stream->request());
    res.setStatus(http::kStatusBadRequest);
    res.addBody("error: missing ?table=... parameter");
    res_stream->writeResponse(res);
    return;
  }

  String partition;
  if (!URI::getParam(params, "partition", &partition)) {
    http::HTTPResponse res;
    res.populateFromRequest(req_stream->request());
    res.setStatus(http::kStatusBadRequest);
    res.addBody("error: missing ?partition=... parameter");
    res_stream->writeResponse(res);
    return;
  }

  String limit_str;
  if (!URI::getParam(params, "limit", &limit_str)) {
    http::HTTPResponse res;
    res.populateFromRequest(req_stream->request());
    res.setStatus(http::kStatusBadRequest);
    res.addBody("error: missing ?limit=... parameter");
    res_stream->writeResponse(res);
    return;
  }

  auto limit = std::stoull(limit_str);
  auto scan_params = msg::decode<EventScanParams>(
      req_stream->request().body());

  EventScanResult result(limit);

  service_->scanLocalTablePartition(
      session,
      table,
      SHA1Hash::fromHexString(partition),
      scan_params,
      &result);

  Buffer res_body;
  {
    BufferedOutputStream res_os(BufferOutputStream::fromBuffer(&res_body));
    result.encode(&res_os);
  }

  http::HTTPResponse res;
  res.populateFromRequest(req_stream->request());
  res.setStatus(http::kStatusOK);
  res.addBody(res_body);
  res_stream->writeResponse(res);
}

}
