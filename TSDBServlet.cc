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
#include <fnord-base/util/Base64.h>

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

    if (StringUtil::endsWith(uri.path(), "/replicate")) {
      return insertRecordsReplication(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/list_chunks")) {
      return listChunks(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/list_files")) {
      return listFiles(req, res, &uri);
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

void TSDBServlet::insertRecordsReplication(
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

  String chunk_base64;
  if (!URI::getParam(params, "chunk", &chunk_base64)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?chunk=... parameter");
    return;
  }

  String chunk;
  util::Base64::decode(chunk_base64, &chunk);

  auto& buf = req->body();
  util::BinaryMessageReader reader(buf.data(), buf.size());
  while (reader.remaining() > 0) {
    auto record_id = *reader.readUInt64();
    auto len = reader.readVarUInt();
    auto data = reader.read(len);
    node_->insertRecord(stream, record_id, Buffer(data, len), chunk);
  }

  res->setStatus(http::kStatusCreated);
}

void TSDBServlet::listChunks(
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

  DateTime from;
  String from_str;
  if (URI::getParam(params, "from", &from_str)) {
    from = DateTime(std::stoul(from_str));
  } else {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?from=... parameter");
    return;
  }

  DateTime until;
  String until_str;
  if (URI::getParam(params, "until", &until_str)) {
    until = DateTime(std::stoul(until_str));
  } else {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?until=... parameter");
    return;
  }

  auto cfg = node_->configFor(stream);
  auto chunks = StreamChunk::streamChunkKeysFor(stream, from, until, *cfg);

  Vector<String> chunks_encoded;
  for (const auto& c : chunks) {
    String encoded;
    util::Base64::encode(c, &encoded);
    chunks_encoded.emplace_back(encoded);
  }

  util::BinaryMessageWriter buf;
  for (const auto& c : chunks_encoded) {
    buf.appendLenencString(c);
  }

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/octet-stream");
  res->addBody(buf.data(), buf.size());
/*


  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream j(res->getBodyOutputStream());

  j.beginObject();
  j.addObjectEntry("stream");
  j.addString(stream);
  j.addComma();
  j.addObjectEntry("from");
  j.addInteger(from.unixMicros());
  j.addComma();
  j.addObjectEntry("until");
  j.addInteger(until.unixMicros());
  j.addComma();
  j.addObjectEntry("chunk_keys");
  json::toJSON(chunks_encoded, &j);
  j.endObject();
*/
}

void TSDBServlet::listFiles(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String chunk;
  if (!URI::getParam(params, "chunk", &chunk)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?chunk=... parameter");
    return;
  }

  String chunk_key;
  util::Base64::decode(chunk, &chunk_key);
  auto files = node_->listFiles(chunk_key);

  util::BinaryMessageWriter buf;
  for (const auto& f : files) {
    buf.appendLenencString(f);
  }

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/octet-stream");
  res->addBody(buf.data(), buf.size());
/*
  Vector<String> chunks_encoded;
  for (const auto& c : chunks) {
    String encoded;
    util::Base64::encode(c, &encoded);
    chunks_encoded.emplace_back(encoded);
  }

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream j(res->getBodyOutputStream());

  j.beginObject();
  j.addObjectEntry("stream");
  j.addString(stream);
  j.addComma();
  j.addObjectEntry("from");
  j.addInteger(from.unixMicros());
  j.addComma();
  j.addObjectEntry("until");
  j.addInteger(until.unixMicros());
  j.addComma();
  j.addObjectEntry("chunk_keys");
  json::toJSON(chunks_encoded, &j);
  j.endObject();
*/
}

}
}

