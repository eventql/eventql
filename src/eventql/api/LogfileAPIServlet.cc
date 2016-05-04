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
