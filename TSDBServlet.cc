/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-base/util/binarymessagewriter.h"
#include "fnord-tsdb/TSDBServlet.h"
#include "fnord-json/json.h"
#include <fnord-base/wallclock.h>
#include "fnord-msg/MessageEncoder.h"
#include "fnord-msg/MessagePrinter.h"

namespace fnord {
namespace tsdb {

TSDBServlet::TSDBServlet(TSDBNode* node) : node_(node) {}

void TSDBServlet::handleHTTPRequest(
    fnord::http::HTTPRequest* req,
    fnord::http::HTTPResponse* res) {
  URI uri(req->uri());

  res->addHeader("Access-Control-Allow-Origin", "*");

  try {
    if (StringUtil::endsWith(uri.path(), "/insert")) {
      return insertRecord(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/insert_batch")) {
      return insertRecordsBatch(req, res, &uri);
    }

    res->setStatus(fnord::http::kStatusNotFound);
    res->addBody("not found");
  } catch (const Exception& e) {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0: $1", e.getTypeName(), e.getMessage()));
  }
}

void TSDBServlet::insertRecord(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String stream;
  if (!URI::getParam(params, "stream", &stream)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?stream=... parameter");
    return;
  }

  uint64_t record_id = rnd_.random64();
  auto time = WallClock::now();

  node_->insertRecord(stream, record_id, req->body(), time);
  res->setStatus(http::kStatusCreated);
}

void TSDBServlet::insertRecordsBatch(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String stream;
  if (!URI::getParam(params, "stream", &stream)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?stream=... parameter");
    return;
  }

  auto& buf = req->body();
  util::BinaryMessageReader reader(buf.data(), buf.size());
  while (reader.remaining() > 0) {
    auto time = *reader.readUInt64();
    auto record_id = *reader.readUInt64();
    auto len = reader.readVarUInt();
    auto data = reader.read(len);
    node_->insertRecord(stream, record_id, Buffer(data, len), time);
  }

  res->setStatus(http::kStatusCreated);
}

}
}

