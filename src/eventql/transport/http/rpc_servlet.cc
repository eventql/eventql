/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/eventql.h"
#include "eventql/util/util/binarymessagewriter.h"
#include "eventql/transport/http/rpc_servlet.h"
#include "eventql/transport/http/http_auth.h"
#include "eventql/db/record_envelope.pb.h"
#include "eventql/util/json/json.h"
#include <eventql/util/wallclock.h>
#include <eventql/util/thread/wakeup.h>
#include "eventql/util/protobuf/MessageEncoder.h"
#include "eventql/util/protobuf/MessagePrinter.h"
#include "eventql/util/protobuf/msg.h"
#include <eventql/util/util/Base64.h>
#include <eventql/util/fnv.h>
#include <eventql/io/sstable/sstablereader.h>
#include "eventql/server/session.h"
#include "eventql/server/sql_service.h"
#include "eventql/server/sql/codec/binary_codec.h"
#include "eventql/sql/runtime/runtime.h"

namespace eventql {

RPCServlet::RPCServlet(Database* database) : db_(database) {}

void RPCServlet::handleHTTPRequest(
    RefPtr<http::HTTPRequestStream> req_stream,
    RefPtr<http::HTTPResponseStream> res_stream) {
  const auto& req = req_stream->request();
  URI uri(req.uri());

  logDebug("eventql", "HTTP Request: $0 $1", req.method(), req.uri());

  auto session = db_->getSession();
  auto dbctx = session->getDatabaseContext();

  http::HTTPResponse res;
  res.populateFromRequest(req);

  res.addHeader("Access-Control-Allow-Origin", "*");
  res.addHeader("Access-Control-Allow-Methods", "GET, POST");
  res.addHeader("Access-Control-Allow-Headers", "X-TSDB-Namespace");

  if (req.method() == http::HTTPMessage::M_OPTIONS) {
    req_stream->readBody();
    res.setStatus(http::kStatusOK);
    res_stream->writeResponse(res);
    return;
  }

  auto auth_rc = dbctx->internal_auth->verifyRequest(session, req);
  if (!auth_rc.isSuccess()) {
    auth_rc = HTTPAuth::authenticateRequest(
        session,
        dbctx->client_auth,
        req);
  }

  try {
    if (uri.path() == "/tsdb/replicate") {
      req_stream->readBody();
      replicateRecords(&req, &res, &uri);
      res_stream->writeResponse(res);
      return;
    }

    res.setStatus(http::kStatusNotFound);
    res.addBody("not found");
    res_stream->writeResponse(res);
  } catch (const Exception& e) {
    try {
      req_stream->readBody();
    } catch (...) {
      /* ignore further errors */
    }

    logError("tsdb", e, "error while processing HTTP request");

    res.setStatus(http::kStatusInternalServerError);
    res.addBody(StringUtil::format("error: $0: $1", e.getTypeName(), e.getMessage()));
    res_stream->writeResponse(res);
  }

  res_stream->finishResponse();
}

void RPCServlet::replicateRecords(
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  auto session = db_->getSession();
  auto dbctx = session->getDatabaseContext();

  const auto& params = uri->queryParams();

  String tsdb_namespace;
  if (!URI::getParam(params, "namespace", &tsdb_namespace)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?namespace=... parameter");
    return;
  }

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  String partition_key;
  if (!URI::getParam(params, "partition", &partition_key)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?partition=... parameter");
    return;
  }

  ShreddedRecordList records;
  auto body_is = req->getBodyInputStream();
  records.decode(body_is.get());

  auto rc = dbctx->table_service->insertReplicatedRecords(
      tsdb_namespace,
      table_name,
      SHA1Hash::fromHexString(partition_key),
      records);

  if (rc.isSuccess()) {
    res->setStatus(http::kStatusCreated);
  } else {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody("ERROR: " + rc.getMessage());
    return;
  }
}

}

