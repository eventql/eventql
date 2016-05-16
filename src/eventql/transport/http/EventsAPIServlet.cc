/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include "eventql/util/wallclock.h"
#include "eventql/util/assets.h"
#include "eventql/util/protobuf/msg.h"
#include "eventql/util/io/BufferedOutputStream.h"
#include "eventql/transport/http/EventsAPIServlet.h"

#include "eventql/eventql.h"

namespace eventql {

EventsAPIServlet::EventsAPIServlet(
    EventsService* service,
    ConfigDirectory* cdir,
    const String& cachedir) :
    service_(service),
    cdir_(cdir),
    cachedir_(cachedir) {}

void EventsAPIServlet::handle(
    const AnalyticsSession& session,
    RefPtr<http::HTTPRequestStream> req_stream,
    RefPtr<http::HTTPResponseStream> res_stream) {
  const auto& req = req_stream->request();
  URI uri(req.uri());

  http::HTTPResponse res;
  res.populateFromRequest(req);

  if (uri.path() == "/transport/http/v1/events/scan") {
    scanTable(session, uri, req_stream.get(), res_stream.get());
    return;
  }

  if (uri.path() == "/transport/http/v1/events/scan_partition") {
    scanTablePartition(session, uri, req_stream.get(), res_stream.get());
    return;
  }

  res.setStatus(http::kStatusNotFound);
  res.addHeader("Content-Type", "text/html; charset=utf-8");
  res.addBody(Assets::getAsset("eventql/webui/404.html"));
  res_stream->writeResponse(res);
}

void EventsAPIServlet::scanTable(
    const AnalyticsSession& session,
    const URI& uri,
    http::HTTPRequestStream* req_stream,
    http::HTTPResponseStream* res_stream) {
  req_stream->readBody();

  const auto& params = uri.queryParams();

  auto sse_stream = mkRef(new http::HTTPSSEStream (req_stream, res_stream));
  sse_stream->start();

  auto send_result_row = [sse_stream] (const msg::DynamicMessage& row) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("event");
    row.toJSON(&json);
    json.endObject();

    sse_stream->sendEvent(buf, Some<String>("result"));
  };

  auto send_status_update = [sse_stream] (bool done) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("status");
    json.addString(done ? "finished" : "running");
    json.endObject();

    sse_stream->sendEvent(buf, Some<String>("progress"));
  };

  try {
    String table_name;
    if (!URI::getParam(params, "table", &table_name)) {
      RAISE(kRuntimeError, "error: missing ?table=... parameter");
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
    scan_params.set_limit(limit);

    String columns_str;
    if (URI::getParam(params, "columns", &columns_str)) {
      for (const auto& c : StringUtil::split(columns_str, ",")) {
        *scan_params.add_columns() = c;
      }
    }

    service_->scanTable(
        session,
        table_name,
        scan_params,
        send_result_row,
        send_status_update);

  } catch (const StandardException& e) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("error");
    json.addString(e.what());
    json.endObject();

    sse_stream->sendEvent(buf, Some<String>("query_error"));
    sse_stream->finish();
    return;
  }

  {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.endObject();

    sse_stream->sendEvent(buf, Some<String>("query_completed"));
  }

  sse_stream->finish();
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

  auto result = service_->scanLocalTablePartition(
      session,
      table,
      SHA1Hash::fromHexString(partition),
      scan_params);

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
