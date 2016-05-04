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
#include "eventql/util/protobuf/msg.h"
#include "eventql/util/io/BufferedOutputStream.h"
#include "eventql/api/LogfileAPIServlet.h"

using namespace stx;

namespace zbase {

LogfileAPIServlet::LogfileAPIServlet(
    LogfileService* service,
    ConfigDirectory* cdir,
    const String& cachedir) :
    service_(service),
    cdir_(cdir),
    cachedir_(cachedir) {}

void LogfileAPIServlet::handle(
    const AnalyticsSession& session,
    RefPtr<stx::http::HTTPRequestStream> req_stream,
    RefPtr<stx::http::HTTPResponseStream> res_stream) {
  const auto& req = req_stream->request();
  URI uri(req.uri());

  http::HTTPResponse res;
  res.populateFromRequest(req);

  if (uri.path() == "/api/v1/logfiles") {
    req_stream->readBody();
    listLogfiles(session, uri, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/logfiles/get_definition") {
    req_stream->readBody();
    fetchLogfileDefinition(session, uri, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/logfiles/set_regex") {
    req_stream->readBody();
    setLogfileRegex(session, uri, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/logfiles/scan") {
    scanLogfile(session, uri, req_stream.get(), res_stream.get());
    return;
  }

  if (uri.path() == "/api/v1/logfiles/scan_partition") {
    scanLogfilePartition(session, uri, req_stream.get(), res_stream.get());
    return;
  }

  if (uri.path() == "/api/v1/logfiles/upload") {
    uploadLogfile(session, uri, req_stream.get(), &res);
    res_stream->writeResponse(res);
    return;
  }

  res.setStatus(http::kStatusNotFound);
  res.addHeader("Content-Type", "text/html; charset=utf-8");
  res.addBody(Assets::getAsset("eventql/webui/404.html"));
  res_stream->writeResponse(res);
}

void LogfileAPIServlet::listLogfiles(
    const AnalyticsSession& session,
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto customer_conf = cdir_->configFor(session.customer());
  const auto& logfile_cfg = customer_conf->config.logfile_import_config();

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  json.beginObject();
  json.addObjectEntry("logfile_definitions");
  json.beginArray();

  size_t nlogs = 0;
  for (const auto& logfile : logfile_cfg.logfiles()) {
    if (++nlogs > 1) {
      json.addComma();
    }

    renderLogfileDefinition(&logfile, &json);
  }

  json.endArray();
  json.endObject();

  res->setStatus(http::kStatusOK);
  res->setHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(buf);
}

void LogfileAPIServlet::fetchLogfileDefinition(
    const AnalyticsSession& session,
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();
  auto customer_conf = cdir_->configFor(session.customer());
  const auto& logfile_cfg = customer_conf->config.logfile_import_config();

  String logfile_name;
  if (!URI::getParam(params, "logfile", &logfile_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?logfile=... parameter");
    return;
  }

  const LogfileDefinition* logfile_def = nullptr;
  for (const auto& logfile : logfile_cfg.logfiles()) {
    if (logfile.name() == logfile_name) {
      logfile_def = &logfile;
      break;
    }
  }

  if (logfile_def == nullptr) {
    res->setStatus(http::kStatusNotFound);
    res->addBody("logfile not found");
  } else {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    renderLogfileDefinition(logfile_def, &json);
    res->setStatus(http::kStatusOK);
    res->setHeader("Content-Type", "application/json; charset=utf-8");
    res->addBody(buf);
  }
}

void LogfileAPIServlet::setLogfileRegex(
    const AnalyticsSession& session,
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();
  auto customer_conf = cdir_->configFor(session.customer());
  const auto& logfile_cfg = customer_conf->config.logfile_import_config();

  String logfile_name;
  if (!URI::getParam(params, "logfile", &logfile_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?logfile=... parameter");
    return;
  }

  String regex_str;
  if (!URI::getParam(params, "regex", &regex_str)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?regex=... parameter");
    return;
  }

  service_->setLogfileRegex(session.customer(), logfile_name, regex_str);

  res->setStatus(http::kStatusCreated);
  res->addBody("ok");
}

void LogfileAPIServlet::renderLogfileDefinition(
    const LogfileDefinition* logfile_def,
    json::JSONOutputStream* json) {
  json->beginObject();

  json->addObjectEntry("name");
  json->addString(logfile_def->name());
  json->addComma();

  json->addObjectEntry("regex");
  json->addString(logfile_def->regex());
  json->addComma();

  json->addObjectEntry("source_fields");
  json->beginArray();
  {
    size_t nfields = 0;
    for (const auto& field : logfile_def->source_fields()) {
      if (++nfields > 1) {
        json->addComma();
      }

      json->beginObject();

      json->addObjectEntry("name");
      json->addString(field.name());
      json->addComma();

      json->addObjectEntry("id");
      json->addInteger(field.id());
      json->addComma();

      json->addObjectEntry("type");
      json->addString(field.type());
      json->addComma();

      json->addObjectEntry("format");
      json->addString(field.format());

      json->endObject();
    }
  }
  json->endArray();
  json->addComma();

  json->addObjectEntry("row_fields");
  json->beginArray();
  {
    size_t nfields = 0;
    for (const auto& field : logfile_def->row_fields()) {
      if (++nfields > 1) {
        json->addComma();
      }

      json->beginObject();

      json->addObjectEntry("name");
      json->addString(field.name());
      json->addComma();

      json->addObjectEntry("id");
      json->addInteger(field.id());
      json->addComma();

      json->addObjectEntry("type");
      json->addString(field.type());
      json->addComma();

      json->addObjectEntry("format");
      json->addString(field.format());

      json->endObject();
    }
  }
  json->endArray();

  json->endObject();
}

void LogfileAPIServlet::scanLogfile(
    const AnalyticsSession& session,
    const URI& uri,
    http::HTTPRequestStream* req_stream,
    http::HTTPResponseStream* res_stream) {
  req_stream->readBody();

  const auto& params = uri.queryParams();

  String logfile_name;
  if (!URI::getParam(params, "logfile", &logfile_name)) {
    http::HTTPResponse res;
    res.populateFromRequest(req_stream->request());
    res.setStatus(http::kStatusBadRequest);
    res.addBody("error: missing ?logfile=... parameter");
    res_stream->writeResponse(res);
    return;
  }

  LogfileScanParams scan_params;

  scan_params.set_end_time(WallClock::unixMicros());
  scan_params.set_scan_type(LOGSCAN_ALL);

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
      if (c == "__raw__") {
        scan_params.set_return_raw(true);
      } else {
        *scan_params.add_columns() = c;
      }
    }
  }

  String filter_str;
  if (URI::getParam(params, "filter", &filter_str) && !filter_str.empty()) {
    scan_params.set_condition(filter_str);
    scan_params.set_scan_type(LOGSCAN_SQL);
  }

  LogfileScanResult result(limit);

  auto sse_stream = mkRef(new http::HTTPSSEStream(req_stream, res_stream));
  sse_stream->start();

  auto send_status_update = [sse_stream, &result, scan_params] (bool done) {
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
    json.addObjectEntry("columns");
    json::toJSON(result.columns(), &json);
    json.addComma();
    json.addObjectEntry("result");
    json.beginArray();

    size_t nline = 0;
    for (const auto& l : result.lines()) {
      if (++nline > 1) {
        json.addComma();
      }

      json.beginObject();
      json.addObjectEntry("time");
      json.addInteger(l.time.unixMicros());
      json.addComma();

      if (scan_params.return_raw()) {
        json.addObjectEntry("raw");
        json.addString(l.raw);
        json.addComma();
      }

      json.addObjectEntry("columns");
      json::toJSON(l.columns, &json);
      json.endObject();
    }

    json.endArray();
    json.endObject();

    sse_stream->sendEvent(buf, None<String>());
  };

  service_->scanLogfile(
      session,
      logfile_name,
      scan_params,
      &result,
      send_status_update);

  sse_stream->finish();
}

void LogfileAPIServlet::scanLogfilePartition(
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
  auto scan_params = msg::decode<LogfileScanParams>(
      req_stream->request().body());

  LogfileScanResult result(limit);

  service_->scanLocalLogfilePartition(
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

void LogfileAPIServlet::uploadLogfile(
    const AnalyticsSession& session,
    const URI& uri,
    http::HTTPRequestStream* req_stream,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();

  if (req_stream->request().method() != http::HTTPMessage::M_POST) {
    req_stream->discardBody();
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: expected HTTP POST request");
    return;
  }

  String logfile_name;
  if (!URI::getParam(params, "logfile", &logfile_name)) {
    req_stream->discardBody();
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?logfile=... parameter");
    return;
  }

  auto logfile_def = service_->findLogfileDefinition(
      session.customer(),
      logfile_name);

  if (logfile_def.isEmpty()) {
    req_stream->discardBody();
    res->setStatus(http::kStatusNotFound);
    res->addBody("error: logfile not found");
    return;
  }

  Vector<Pair<String, String>> source_fields;
  for (const auto& source_field : logfile_def.get().source_fields()) {
    String field_val;

    if (!URI::getParam(params, source_field.name(), &field_val)) {
      req_stream->discardBody();
      res->setStatus(http::kStatusBadRequest);
      res->addBody(
          StringUtil::format(
              "error: missing ?$0=... parameter",
              source_field.name()));
      return;
    }

    source_fields.emplace_back(source_field.name(), field_val);
  }

  auto tmpfile_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("upload_$0.tmp", Random::singleton()->hex128()));

  auto tmpfile = File::openFile(
      tmpfile_path,
      File::O_CREATE | File::O_READ | File::O_WRITE | File::O_AUTODELETE);

  size_t body_size = 0;
  req_stream->readBody([&tmpfile, &body_size] (const void* data, size_t size) {
    tmpfile.write(data, size);
    body_size += size;
  });

  tmpfile.seekTo(0);

  auto is = FileInputStream::fromFile(std::move(tmpfile));

  service_->insertLoglines(
      session.customer(),
      logfile_def.get(),
      source_fields,
      is.get());

  res->setStatus(http::kStatusCreated);
}

}
