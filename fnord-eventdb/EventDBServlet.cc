/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-eventdb/EventDBServlet.h"
#include "fnord-json/json.h"

namespace fnord {
namespace eventdb {

EventDBServlet::EventDBServlet(TableRepository* tables) : tables_(tables) {}

void EventDBServlet::handleHTTPRequest(
    fnord::http::HTTPRequest* req,
    fnord::http::HTTPResponse* res) {
  URI uri(req->uri());

  res->addHeader("Access-Control-Allow-Origin", "*");

  try {
    if (StringUtil::endsWith(uri.path(), "/insert")) {
      return insertRecord(req, res, &uri);
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

    res->setStatus(fnord::http::kStatusNotFound);
    res->addBody("not found");
  } catch (const Exception& e) {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0", e.getMessage()));
  }
}

void EventDBServlet::insertRecord(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);
  tbl->addRecords(req->body());

  res->setStatus(http::kStatusCreated);
}

void EventDBServlet::commitTable(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);
  auto n = tbl->commit();

  res->setStatus(http::kStatusOK);
  res->addBody(StringUtil::toString(n));
}

void EventDBServlet::mergeTable(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);
  tbl->merge();

  res->setStatus(http::kStatusOK);
}

void EventDBServlet::gcTable(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto tbl = tables_->findTableWriter(table);
  tbl->gc();

  res->setStatus(http::kStatusOK);
}

void EventDBServlet::tableInfo(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  String table;
  if (!URI::getParam(params, "table", &table)) {
    res->setStatus(fnord::http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto snap = tables_->getSnapshot(table);

  uint64_t num_rows_commited = 0;
  uint64_t num_rows_arena = 0;
  uint64_t head_sequence = 0;

  for (const auto& c : snap->head->chunks) {
    num_rows_commited += c.num_records;

    auto seq = (c.start_sequence + c.num_records) - 1;
    if (seq > head_sequence) {
      head_sequence = seq;
    }
  }

  for (const auto& a : snap->arenas) {
    if (a->startSequence() > head_sequence) {
      num_rows_arena += a->size();
    }
  }

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream json(res->getBodyOutputStream());

  json.beginObject();
  json.addObjectEntry("table");
  json.addString(table);
  json.addComma();
  json.addObjectEntry("num_chunks");
  json.addInteger(snap->head->chunks.size());
  json.addComma();
  json.addObjectEntry("head_sequence");
  json.addInteger(head_sequence);
  json.addComma();
  json.addObjectEntry("num_rows");
  json.addInteger(num_rows_commited + num_rows_arena);
  json.addComma();
  json.addObjectEntry("num_rows_commited");
  json.addInteger(num_rows_commited);
  json.addComma();
  json.addObjectEntry("num_rows_stage");
  json.addInteger(num_rows_arena);
  json.endObject();
}

}
}
