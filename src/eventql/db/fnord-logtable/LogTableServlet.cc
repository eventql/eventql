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
#include "eventql/util/util/binarymessagewriter.h"
#include "fnord-logtable/LogTableServlet.h"
#include "eventql/util/json/json.h"
#include "eventql/util/protobuf/MessageEncoder.h"
#include "eventql/util/protobuf/MessagePrinter.h"

namespace util {
namespace logtable {

LogTableServlet::LogTableServlet(TableRepository* tables) : tables_(tables) {}

void LogTableServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());

  res->addHeader("Access-Control-Allow-Origin", "*");

  try {
    if (StringUtil::endsWith(uri.path(), "/insert")) {
      return insertRecord(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/insert_batch")) {
      return insertRecordsBatch(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/fetch")) {
      return fetchRecord(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/fetch_batch")) {
      return fetchRecordsBatch(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/commit")) {
      return commitTable(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/merge")) {
      return mergeTable(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/gc")) {
      return gcTable(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/info")) {
      return tableInfo(req, res, &uri);
    }

    if (StringUtil::endsWith(uri.path(), "/snapshot")) {
      return tableSnapshot(req, res, &uri);
    }

    res->setStatus(http::kStatusNotFound);
    res->addBody("not found");
  } catch (const Exception& e) {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0: $1", e.getTypeName(), e.getMessage()));
  }
}

void LogTableServlet::insertRecord(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);

  if (tbl->arenaSize() > 50000) {
    res->setStatus(http::kStatusServiceUnavailable);
    res->addBody("too many uncommitted records, retry in a few seconds");
    return;
  }

  tbl->addRecord(req->body());
  res->setStatus(http::kStatusCreated);
}

void LogTableServlet::insertRecordsBatch(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);

  if (tbl->arenaSize() > 50000) {
    res->setStatus(http::kStatusServiceUnavailable);
    res->addBody("too many uncommitted records, retry in a few seconds");
    return;
  }

  auto& buf = req->body();
  util::BinaryMessageReader reader(buf.data(), buf.size());
  while (reader.remaining() > 0) {
    auto len = reader.readVarUInt();
    auto data = reader.read(len);
    tbl->addRecord(Buffer(data, len));
  }

  res->setStatus(http::kStatusCreated);
}

void LogTableServlet::commitTable(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);
  auto n = tbl->commit();

  res->setStatus(http::kStatusOK);
  res->addBody(StringUtil::toString(n));
}

void LogTableServlet::mergeTable(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);
  tbl->merge();

  res->setStatus(http::kStatusOK);
}

void LogTableServlet::gcTable(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);
  tbl->gc();

  res->setStatus(http::kStatusOK);
}

void LogTableServlet::tableInfo(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);
  auto snap = tables_->getSnapshot(table);

  HashMap<String, uint64_t> per_replica_head;
  HashMap<String, uint64_t> artifacts;
  HashMap<String, HashMap<String, String>> per_replica;
  uint64_t num_recs_commited = 0;
  uint64_t num_recs_arena = 0;

  for (const auto& c : snap->head->chunks) {
    auto chunkname = table + "." + c.replica_id + "." + c.chunk_id;

    num_recs_commited += c.num_records;
    artifacts[chunkname] = c.num_records;

    auto seq = (c.start_sequence + c.num_records) - 1;
    if (seq > per_replica_head[c.replica_id]) {
      per_replica_head[c.replica_id] = seq;
      per_replica[c.replica_id]["head_sequence"] = StringUtil::toString(seq);
    }
  }

  auto my_head = per_replica_head[tables_->replicaID()];
  for (const auto& a : snap->arenas) {
    if (a->startSequence() > my_head) {
      num_recs_arena += a->size();
    }
  }


  Vector<String> missing_chunks;
  uint64_t bytes_total = 0;
  uint64_t bytes_present = 0;
  uint64_t bytes_downloading = 0;
  uint64_t bytes_missing = 0;
  uint64_t num_chunks_present = 0;
  uint64_t num_chunks_downloading = 0;
  uint64_t num_chunks_missing = 0;
  uint64_t num_recs_present = 0;
  uint64_t num_recs_downloading = 0;
  uint64_t num_recs_missing = 0;

  auto artifactlist = tbl->artifactIndex()->listArtifacts();
  for (const auto& a : artifactlist) {
    if (artifacts.count(a.name) > 0) {
      bytes_total += a.totalSize();

      switch (a.status) {

        case ArtifactStatus::PRESENT:
          bytes_present += a.totalSize();
          ++num_chunks_present;
          num_recs_present += artifacts[a.name];
          break;

        case ArtifactStatus::DOWNLOAD:
          bytes_downloading += a.totalSize();
          ++num_chunks_downloading;
          num_recs_downloading += artifacts[a.name];
          break;

        default:
        case ArtifactStatus::MISSING:
          missing_chunks.emplace_back(a.name);
          bytes_missing += a.totalSize();
          ++num_chunks_missing;
          num_recs_missing += artifacts[a.name];
          break;

      }
    }
  }

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream j(res->getBodyOutputStream());

  j.beginObject();
  j.addObjectEntry("table");
  j.addString(table);
  j.addComma();
  j.addObjectEntry("num_records_total");
  j.addInteger(num_recs_commited + num_recs_arena);
  j.addComma();
  j.addObjectEntry("num_records_commited");
  j.addInteger(num_recs_commited);
  j.addComma();
  j.addObjectEntry("num_records_present");
  j.addInteger(num_recs_present);
  j.addComma();
  j.addObjectEntry("num_records_downloading");
  j.addInteger(num_recs_downloading);
  j.addComma();
  j.addObjectEntry("num_records_stage");
  j.addInteger(num_recs_arena);
  j.addComma();
  j.addObjectEntry("num_records_missing");
  j.addInteger(num_recs_missing);
  j.addComma();
  j.addObjectEntry("bytes_total");
  j.addInteger(bytes_total);
  j.addComma();
  j.addObjectEntry("bytes_present");
  j.addInteger(bytes_present);
  j.addComma();
  j.addObjectEntry("bytes_downloading");
  j.addInteger(bytes_downloading);
  j.addComma();
  j.addObjectEntry("bytes_missing");
  j.addInteger(bytes_missing);
  j.addComma();
  j.addObjectEntry("num_chunks");
  j.addInteger(snap->head->chunks.size());
  j.addComma();
  j.addObjectEntry("num_chunks_present");
  j.addInteger(num_chunks_present);
  j.addComma();
  j.addObjectEntry("num_chunks_downloading");
  j.addInteger(num_chunks_downloading);
  j.addComma();
  j.addObjectEntry("num_chunks_missing");
  j.addInteger(num_chunks_missing);
  j.addComma();
  j.addObjectEntry("replicas");
  json::toJSON(per_replica, &j);
  j.addComma();
  j.addObjectEntry("missing_chunks");
  json::toJSON(missing_chunks, &j);
  j.endObject();
}

void LogTableServlet::tableSnapshot(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto snap = tables_->getSnapshot(table);

  Buffer buf;
  snap->head->encode(&buf);

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/octet-stream");
  res->addBody(buf);
}

void LogTableServlet::fetchRecord(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  String replica;
  if (!URI::getParam(params, "replica", &replica)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?replica=... parameter");
    return;
  }

  String seq_str;
  uint64_t seq;
  if (URI::getParam(params, "seq", &seq_str)) {
    seq = std::stoull(seq_str);
  } else {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?seq=... parameter");
    return;
  }

  String format = "binary";
  URI::getParam(params, "format", &format);

  auto tbl = tables_->findTableReader(table);
  const auto& schema = tbl->schema();

  Buffer body;

  if (format == "human") {
    tbl->fetchRecords(
        replica,
        seq,
        1,
        [&body, &schema] (const msg::MessageObject& rec) -> bool {
          body.append(msg::MessagePrinter::print(rec, schema));
          return true;
        });

    res->addHeader("Content-Type", "text/plain");
  } else {
    tbl->fetchRecords(
        replica,
        seq,
        1,
        [&body, &schema] (const msg::MessageObject& rec) -> bool {
          msg::MessageEncoder::encode(rec, schema, &body);
          return true;
        });

    res->addHeader("Content-Type", "application/octet-stream");
  }

  res->setStatus(http::kStatusOK);
  res->addBody(body);
}

void LogTableServlet::fetchRecordsBatch(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  String replica;
  if (!URI::getParam(params, "replica", &replica)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?replica=... parameter");
    return;
  }

  String seq_str;
  uint64_t seq;
  if (URI::getParam(params, "seq", &seq_str)) {
    seq = std::stoull(seq_str);
  } else {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?seq=... parameter");
    return;
  }

  String limit_str;
  uint64_t limit;
  if (URI::getParam(params, "limit", &limit_str)) {
    limit = std::stoull(limit_str);
  } else {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?limit=... parameter");
    return;
  }

  String format = "binary";
  URI::getParam(params, "format", &format);

  auto tbl = tables_->findTableReader(table);
  const auto& schema = tbl->schema();

  util::BinaryMessageWriter body;

  if (format == "human") {
    tbl->fetchRecords(
        replica,
        seq,
        limit,
        [&body, &schema] (const msg::MessageObject& rec) -> bool {
          auto str = msg::MessagePrinter::print(rec, schema);
          body.append(str.data(), str.size());
          return true;
        });

    res->addHeader("Content-Type", "text/plain");
  } else {
    tbl->fetchRecords(
        replica,
        seq,
        limit,
        [&body, &schema] (const msg::MessageObject& rec) -> bool {
          Buffer buf;
          msg::MessageEncoder::encode(rec, schema, &buf);
          body.appendVarUInt(buf.size());
          body.append(buf.data(), buf.size());
          return true;
        });

    res->addHeader("Content-Type", "application/octet-stream");
  }

  res->setStatus(http::kStatusOK);
  res->addBody(body.data(), body.size());
}

}
}
