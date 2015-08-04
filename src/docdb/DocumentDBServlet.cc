/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <thread>

#include "DocumentDBServlet.h"
#include "analytics/CTRCounter.h"
#include "stx/Language.h"
#include "stx/wallclock.h"
#include "stx/io/fileutil.h"
#include "stx/util/Base64.h"
#include "stx/logging.h"
#include "stx/http/cookies.h"
#include "stx/protobuf/DynamicMessage.h"
#include "stx/protobuf/MessageEncoder.h"
#include "stx/csv/CSVInputStream.h"
#include "stx/csv/BinaryCSVInputStream.h"
#include "analytics/TrafficSegment.h"
#include "analytics/AnalyticsQuery.h"
#include "analytics/AnalyticsQueryResult.h"
#include "analytics/AnalyticsQueryReducer.h"
#include "analytics/Report.h"
#include "analytics/StaticTableBuilder.h"
#include "analytics/AnalyticsQueryParams.pb.h"
#include "analytics/PipelineInfo.h"
#include "common/TableDefinition.h"
#include "chartsql/runtime/ASCIITableFormat.h"
#include "chartsql/runtime/JSONSSEStreamFormat.h"
#include "tsdb/TimeWindowPartitioner.h"

using namespace stx;

namespace cm {

DocumentDBServlet::DocumentDBServlet(
    DocumentDB* docdb) :
    dodocumentdb_(docdb) {}

void DocumentDBServlet::handleHTTPRequest(
    stx::http::HTTPRequest* req,
    stx::http::HTTPResponse* res) override;
  URI uri(req.uri());

  /* auth */
  auto session_opt = auth_->authenticateRequest(req_stream->request());
  if (session_opt.isEmpty()) {
    req_stream->readBody();
    res.addHeader("WWW-Authenticate", "Token");
    res.setStatus(http::kStatusUnauthorized);
    res.addBody("authorization required");
    res_stream->writeResponse(res);
    return;
  }

  const auto& session = session_opt.get();

  /* authenticated actions */
  if (uri.path() == "/analytics/api/v1/documents") {
    listDocuments(session, req, res);
    return;
  }

  if (StringUtil::beginsWith(uri.path(), "/analytics/api/v1/documents/")) {
    documentREST(uri, session, req, res);
    return;
  }
}

void DocumentDBServlet::documentREST(
    const URI& uri,
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  static String prefix = "/analytics/api/v1/documents/";
  auto path_parts = StringUtil::split(uri.path().substr(prefix.size()), "/");

  switch (req->method()) {

    case http::HTTPMessage::M_GET:
      switch (path_parts.size()) {
        case 1:
          //listDocuments(path_parts[0], uri, req, res);
          return;
        case 2:
          fetchDocument(
              path_parts[0],
              SHA1Hash::fromHexString(path_parts[1]),
              uri,
              session,
              req,
              res);
          return;
        default:
          break;
      }
      break;

    case http::HTTPMessage::M_POST:
      switch (path_parts.size()) {
        case 1:
          createDocument(path_parts[0], uri, session, req, res);
          return;
        case 2:
          updateDocument(
              path_parts[0],
              SHA1Hash::fromHexString(path_parts[1]),
              uri,
              session,
              req,
              res);
          return;
        default:
          break;
      }
      break;

    default:
      break;

  }

  res->setStatus(http::kStatusBadRequest);
  res->addBody("invalid request");
}

void DocumentDBServlet::fetchDocument(
    const String& type,
    const SHA1Hash& uuid,
    const URI& uri,
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (type == "sql_queries") {
    fetchSQLQuery(uuid, session, req, res);
    return;
  }

  res->setStatus(http::kStatusBadRequest);
  res->addBody("invalid request");
}

void DocumentDBServlet::createDocument(
    const String& type,
    const URI& uri,
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (type == "sql_queries") {
    createSQLQuery(session, req, res);
    return;
  }

  res->setStatus(http::kStatusBadRequest);
  res->addBody("invalid request");
}

void DocumentDBServlet::updateDocument(
    const String& type,
    const SHA1Hash& uuid,
    const URI& uri,
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (type == "sql_queries") {
    updateSQLQuery(uuid, session, req, res);
    return;
  }

  res->setStatus(http::kStatusBadRequest);
  res->addBody("invalid request");
}

void DocumentDBServlet::listDocuments(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));

  json.beginObject();
  json.addObjectEntry("documents");
  json.beginArray();

  size_t i = 0;
  documentdb_->listDocuments(
      session.customer(),
      session.userid(),
      [&i, &json, &session] (const Document& doc) -> bool {
    if (++i > 1) {
      json.addComma();
    }

    json.beginObject();

    json.addObjectEntry("uuid");
    json.addString(doc.uuid());
    json.addComma();

    json.addObjectEntry("name");
    json.addString(doc.name());
    json.addComma();

    json.addObjectEntry("type");
    json.addString(doc.type());

    json.endObject();

    return true;
  });

  json.endArray();
  json.endObject();

  res->setStatus(http::kStatusOK);
  res->setHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(buf);
}

void DocumentDBServlet::fetchSQLQuery(
    const SHA1Hash& uuid,
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  Document doc;
  if (documentdb_->fetchDocument(session.customer(), session.userid(), uuid, &doc)) {
    Buffer buf;
    renderSQLQuery(doc, &buf);
    res->setStatus(http::kStatusOK);
    res->addBody(buf);
  } else {
    res->setStatus(http::kStatusNotFound);
    res->addBody("not found");
  }
}

void DocumentDBServlet::createSQLQuery(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto db_namespace = req->getHeader("X-Analytics-Namespace");
  auto now = WallClock::unixMicros();

  Document doc;
  doc.set_uuid(Random::singleton()->sha1().toString());
  doc.set_name("Unnamed SQL Query"); // FIXPAUL include number
  doc.set_type("sql_query");
  doc.set_version(0);
  doc.set_ctime(now);
  doc.set_mtime(now);
  doc.set_atime(now);

  setDefaultDocumentACLs(&doc, session.userid());

  documentdb_->createDocument(session.customer(), doc);

  Buffer buf;
  renderSQLQuery(doc, &buf);
  res->setStatus(http::kStatusCreated);
  res->addBody(buf);
}

void DocumentDBServlet::updateSQLQuery(
    const SHA1Hash& uuid,
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI::ParamList params;
  URI::parseQueryString(req->body().toString(), &params);

  Vector<Function<void ()>> tx;

  for (const auto& p : params) {

    if (p.first == "content") {
      if (!p.second.empty()) {
        tx.emplace_back([this, &session, &uuid, p] () {
          documentdb_->updateDocumentContent(
              session.customer(),
              session.userid(),
              uuid,
              p.second);
        });
      }

      continue;
    }

    if (p.first == "name") {
      if (!p.second.empty()) {
        tx.emplace_back([this, &session, &uuid, p] () {
          documentdb_->updateDocumentName(
              session.customer(),
              session.userid(),
              uuid,
              p.second);
        });
      }

      continue;
    }

    res->setStatus(http::kStatusBadRequest);
    res->addBody(StringUtil::format("invalid parameter: '$0'", p.first));
    return;
  }

  for (const auto& fn : tx) {
    fn();
  }

  res->setStatus(http::kStatusCreated);
}

void DocumentDBServlet::renderSQLQuery(const Document& doc, Buffer* buf) {
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(buf));

  json.beginObject();

  json.addObjectEntry("uuid");
  json.addString(doc.uuid());
  json.addComma();

  json.addObjectEntry("name");
  json.addString(doc.name());
  json.addComma();

  json.addObjectEntry("sql_query");
  json.addString(doc.content());

  json.endObject();
}

} // namespace cm
