/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <thread>

#include "eventql/eventql.h"
#include "eventql/transport/http/api_servlet.h"
#include "eventql/util/Language.h"
#include "eventql/util/human.h"
#include "eventql/util/wallclock.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/util/Base64.h"
#include "eventql/util/logging.h"
#include "eventql/util/assets.h"
#include "eventql/util/protobuf/msg.h"
#include "eventql/util/http/cookies.h"
#include "eventql/util/protobuf/DynamicMessage.h"
#include "eventql/util/protobuf/MessageEncoder.h"
#include "eventql/util/csv/CSVInputStream.h"
#include "eventql/util/csv/BinaryCSVInputStream.h"
#include "eventql/db/table_config.pb.h"
#include "eventql/server/sql/codec/json_codec.h"
#include "eventql/server/sql/codec/json_sse_codec.h"
#include "eventql/server/sql/codec/binary_codec.h"
#include "eventql/transport/http/http_auth.h"
#include <eventql/io/cstable/cstable_writer.h>
#include <eventql/io/cstable/RecordShredder.h>

namespace eventql {

APIServlet::APIServlet(Database* db) : db_(db) {}

void APIServlet::handleHTTPRequest(
    RefPtr<http::HTTPRequestStream> req_stream,
    RefPtr<http::HTTPResponseStream> res_stream) {
  try {
    handle(req_stream, res_stream);
  } catch (const StandardException& e) {
    logError("eventql", e, "error while handling HTTP request");
    req_stream->discardBody();

    http::HTTPResponse res;
    res.populateFromRequest(req_stream->request());
    res.setStatus(http::kStatusInternalServerError);
    res.addBody(e.what());
    res_stream->writeResponse(res);
  }
}

static void catchAndReturnErrors(
    http::HTTPResponse* resp,
    Function<void ()> fn) {
  try {
    fn();
  } catch (const StandardException& e) {
    resp->setStatus(http::kStatusInternalServerError);
    resp->addBody(e.what());
  }
}

void APIServlet::handle(
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
  res.addHeader("Access-Control-Allow-Headers", "Authorization");

  if (req.method() == http::HTTPMessage::M_OPTIONS) {
    req_stream->readBody();
    res.setStatus(http::kStatusOK);
    res_stream->writeResponse(res);
    return;
  }

  auto auth_rc = HTTPAuth::authenticateRequest(
        session,
        dbctx->client_auth,
        req);

  if (!auth_rc.isSuccess() &&
      !dbctx->config->getBool("cluster.allow_anonymous")) {
    res.setStatus(http::kStatusForbidden);
    res.addHeader("WWW-Authenticate", "Token");
    res.addHeader("Content-Type", "text/plain; charset=utf-8");
    res.addBody(auth_rc.message());
    res_stream->writeResponse(res);
    return;
  }

  if (session->getEffectiveNamespace().empty() &&
      !dbctx->config->getBool("cluster.allow_anonymous")) {
    res.setStatus(http::kStatusUnauthorized);
    res.addHeader("WWW-Authenticate", "Token");
    res.addHeader("Content-Type", "text/plain; charset=utf-8");
    res.addBody("unauthorized");
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/insert") {
    catchAndReturnErrors(&res, [this, session, &req, &res] {
      insertIntoTable(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  /* SQL */
  if (uri.path() == "/api/v1/sql" ||
      uri.path() == "/api/v1/sql_stream") {
    executeSQL(session, &req, &res, res_stream);
    return;
  }

  if (uri.path() == "/api/v1/auth/info") {
    getAuthInfo(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (StringUtil::beginsWith(uri.path(), "/api/v1/mapreduce")) {
    mapreduce_api_.handle(session, req_stream, res_stream);
    return;
  }

  /* TABLES */
  if (uri.path() == "/api/v1/tables/list") {
    catchAndReturnErrors(&res, [this, session, &req, &res] {
      listTables(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/create") {
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      createTable(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/add_field") {
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      addTableField(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/remove_field") {
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      removeTableField(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/drop") {
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      dropTable(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/describe") {
    fetchTableDefinition(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  res.setStatus(http::kStatusNotFound);
  res.addHeader("Content-Type", "text/html; charset=utf-8");
  res.addBody("not found");
  res_stream->writeResponse(res);
}

void APIServlet::listTables(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (req->method() != http::HTTPMessage::kHTTPMethod::M_POST) {
    res->setStatus(http::kStatusMethodNotAllowed);
    res->addHeader("Content-Type", "text/plain; charset=utf-8");
    res->addBody("expected POST request");
    return;
  }

  auto dbctx = session->getDatabaseContext();
  auto jreq = json::parseJSON(req->body());

  /* database */
  auto database = json::objectGetString(jreq, "database");
  if (!database.isEmpty()) {
    auto auth_rc = dbctx->client_auth->changeNamespace(session, database.get());
    if (!auth_rc.isSuccess()) {
      res->setStatus(http::kStatusForbidden);
      res->addHeader("Content-Type", "text/plain; charset=utf-8");
      res->addBody(auth_rc.message());
      return;
    }
  }

  if (session->getEffectiveNamespace().empty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addHeader("Content-Type", "text/plain; charset=utf-8");
    res->addBody("no database selected");
    return;
  }

  /* param tag */
  auto tag_filter_opt = json::objectGetString(jreq, "tag");
  String tag_filter;
  if (!tag_filter_opt.isEmpty() && tag_filter_opt.get() != "all") {
    tag_filter = tag_filter_opt.get();
  }

  /* param sort_fn */
  auto order_filter = json::objectGetString(jreq, "order");

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));

  json.beginObject();
  json.addObjectEntry("tables");
  json.beginArray();

  size_t ntable = 0;

  auto writeTableJSON = [&json, &ntable] (const TableDefinition& table) {
    if (++ntable > 1) {
      json.addComma();
    }

    json.beginObject();

    json.addObjectEntry("name");
    json.addString(table.table_name());
    json.addComma();

    json.endObject();

  };

  dbctx->table_service->listTables(
      session->getEffectiveNamespace(),
      writeTableJSON);

  json.endArray();
  json.endObject();

  res->setStatus(http::kStatusOK);
  res->setHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(buf);
}

void APIServlet::fetchTableDefinition(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (req->method() != http::HTTPMessage::kHTTPMethod::M_POST) {
    res->setStatus(http::kStatusMethodNotAllowed);
    res->addHeader("Content-Type", "text/plain; charset=utf-8");
    res->addBody("expected POST request");
    return;
  }

  auto dbctx = session->getDatabaseContext();
  auto jreq = json::parseJSON(req->body());

  /* database */
  auto database = json::objectGetString(jreq, "database");
  if (!database.isEmpty()) {
    auto auth_rc = dbctx->client_auth->changeNamespace(session, database.get());
    if (!auth_rc.isSuccess()) {
      res->setStatus(http::kStatusForbidden);
      res->addHeader("Content-Type", "text/plain; charset=utf-8");
      res->addBody(auth_rc.message());
      return;
    }
  }

  if (session->getEffectiveNamespace().empty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addHeader("Content-Type", "text/plain; charset=utf-8");
    res->addBody("no database selected");
    return;
  }

  auto table_name = json::objectGetString(jreq, "table");
  if (table_name.isEmpty()) {
    RAISE(kRuntimeError, "missing field: table");
  }

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  auto schema = dbctx->table_service->tableSchema(
      session->getEffectiveNamespace(),
      table_name.get());
  if (schema.isEmpty()) {
    res->setStatus(http::kStatusNotFound);
    res->addBody("table not found");
    return;
  }

  schema.get()->toJSON(&json);

  res->setStatus(http::kStatusOK);
  res->setHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(buf);
}

static ReturnCode tableSchemaFromJSON(
    msg::MessageSchema* schema,
    json::JSONObject::const_iterator begin,
    json::JSONObject::const_iterator end,
    size_t* id) {
  auto ncols = json::arrayLength(begin, end);

  for (size_t i = 0; i < ncols; ++i) {
    auto col = json::arrayLookup(begin, end, i); // O(N^2) but who cares...

    auto name = json::objectGetString(col, end, "name");
    if (name.isEmpty()) {
      return ReturnCode::error("ERUNTIME", "missing field: name");
    }

    auto type = json::objectGetString(col, end, "type");
    if (type.isEmpty()) {
      return ReturnCode::error("ERUNTIME", "missing field: type");
    }

    auto optional = json::objectGetBool(col, end, "optional");
    auto repeated = json::objectGetBool(col, end, "repeated");

    auto field_type = msg::fieldTypeFromString(type.get());

    if (field_type == msg::FieldType::OBJECT) {
      auto child_schema_json = json::objectLookup(col, end, "columns");
      if (child_schema_json == end) {
        return ReturnCode::error("ERUNTIME", "missing field: columns");
      }

      auto child_schema = new msg::MessageSchema(nullptr);
      auto rc = tableSchemaFromJSON(child_schema, child_schema_json, end, id);

      schema->addField(
          msg::MessageSchemaField::mkObjectField(
              ++(*id),
              name.get(),
              repeated.isEmpty() ? false : repeated.get(),
              optional.isEmpty() ? false : optional.get(),
              child_schema));
    } else {
      schema->addField(
          msg::MessageSchemaField(
              ++(*id),
              name.get(),
              field_type,
              0,
              repeated.isEmpty() ? false : repeated.get(),
              optional.isEmpty() ? false : optional.get()));
    }
  }

  return ReturnCode::success();
}

void APIServlet::createTable(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (req->method() != http::HTTPMessage::kHTTPMethod::M_POST) {
    res->setStatus(http::kStatusMethodNotAllowed);
    res->addHeader("Content-Type", "text/plain; charset=utf-8");
    res->addBody("expected POST request");
    return;
  }


  auto dbctx = session->getDatabaseContext();
  auto jreq = json::parseJSON(req->body());

  auto database = json::objectGetString(jreq, "database");
  if (!database.isEmpty()) {
    auto auth_rc = dbctx->client_auth->changeNamespace(session, database.get());
    if (!auth_rc.isSuccess()) {
      res->setStatus(http::kStatusForbidden);
      res->addHeader("Content-Type", "text/plain; charset=utf-8");
      res->addBody(auth_rc.message());
      return;
    }
  }

  if (session->getEffectiveNamespace().empty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addHeader("Content-Type", "text/plain; charset=utf-8");
    res->addBody("no database selected");
    return;
  }

  auto table_name = json::objectGetString(jreq, "table_name");
  if (table_name.isEmpty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing field: table_name");
    return;
  }

  auto jcolumns = json::objectLookup(jreq, "columns");
  if (jcolumns == jreq.end()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing field: columns");
    return;
  }

  auto jpkey = json::objectLookup(jreq, "primary_key");
  if (jpkey == jreq.end()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing field: primary_key");
    return;
  }

  std::vector<std::string> primary_key;
  auto primary_key_count = json::arrayLength(jpkey, jreq.end());
  for (size_t i = 0; i < primary_key_count; ++i) {
    auto pkey_part = json::arrayGetString(jpkey, jreq.end(), i);
    if (pkey_part.isEmpty()) {
      res->setStatus(http::kStatusBadRequest);
      res->addBody("invalid field: primary_key");
      return;
    }

    primary_key.emplace_back(pkey_part.get());
  }

  std::vector<std::pair<std::string, std::string>> properties;
  auto jprops = json::objectLookup(jreq, "properties");
  if (jprops != jreq.end()) {
    auto props_count = json::arrayLength(jprops, jreq.end());
    for (size_t i = 0; i < props_count; ++i) {
      auto jprop = json::arrayLookup(jprops, jreq.end(), i);
      if (jprop == jreq.end()) {
        res->setStatus(http::kStatusBadRequest);
        res->addBody("invalid field: properties");
        return;
      }

      auto prop_key = json::arrayGetString(jprop, jreq.end(), 0);
      auto prop_value = json::arrayGetString(jprop, jreq.end(), 1);
      if (prop_key.isEmpty() || prop_value.isEmpty()) {
        res->setStatus(http::kStatusBadRequest);
        res->addBody("invalid field: properties");
        return;
      }

      properties.emplace_back(prop_key.get(), prop_value.get());
    }
  }

  /* build table schema */
  msg::MessageSchema schema(nullptr);
  size_t id = 0;
  auto schema_rc = tableSchemaFromJSON(&schema, jcolumns, jreq.end(), &id);
  if (!schema_rc.isSuccess()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody(schema_rc.getMessage());
    return;
  }

  auto rc = dbctx->table_service->createTable(
      session->getEffectiveNamespace(),
      table_name.get(),
      schema,
      primary_key,
      properties);

  if (!rc.isSuccess()) {
    logError("eventql", rc.message(), "error");
    res->setStatus(http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0", rc.message()));
    return;
  }

  res->setStatus(http::kStatusCreated);
}

void APIServlet::addTableField(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (req->method() != http::HTTPMessage::kHTTPMethod::M_POST) {
    res->setStatus(http::kStatusMethodNotAllowed);
    res->addHeader("Content-Type", "text/plain; charset=utf-8");
    res->addBody("expected POST request");
    return;
  }

  auto dbctx = session->getDatabaseContext();
  auto jreq = json::parseJSON(req->body());

  /* database */
  auto database = json::objectGetString(jreq, "database");
  if (!database.isEmpty()) {
    auto auth_rc = dbctx->client_auth->changeNamespace(session, database.get());
    if (!auth_rc.isSuccess()) {
      res->setStatus(http::kStatusForbidden);
      res->addHeader("Content-Type", "text/plain; charset=utf-8");
      res->addBody(auth_rc.message());
      return;
    }
  }

  if (session->getEffectiveNamespace().empty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addHeader("Content-Type", "text/plain; charset=utf-8");
    res->addBody("no database selected");
    return;
  }

  auto table_name = json::objectGetString(jreq, "table");
  if (table_name.isEmpty()) {
  res->setStatus(http::kStatusBadRequest);
    res->addBody("missing field: table");
    return;
  }

  auto field_name = json::objectGetString(jreq, "field_name");
  if (field_name.isEmpty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing field: field_name");
    return;
  }

  auto field_type_str = json::objectGetString(jreq, "field_type");
  if (field_type_str.isEmpty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing field: field_type");
    return;
  }

  auto repeated = json::objectGetBool(jreq, "repeated");
  auto optional = json::objectGetBool(jreq, "optional");

  Vector<TableService::AlterTableOperation> operations;
  TableService::AlterTableOperation operation;
  operation.optype = TableService::AlterTableOperationType::OP_ADD_COLUMN;
  operation.field_name = field_name.get();
  operation.field_type = msg::fieldTypeFromString(field_type_str.get());
  operation.is_repeated = repeated.isEmpty() ? false : repeated.get();
  operation.is_optional =  optional.isEmpty() ? false : optional.get();
  operations.emplace_back(operation);

  auto rc = dbctx->table_service->alterTable(
      session->getEffectiveNamespace(),
      table_name.get(),
      operations);

  if (!rc.isSuccess()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody(StringUtil::format("error: $0", rc.message()));
    return;
  }

  res->setStatus(http::kStatusCreated);
  res->addBody("ok");
  return;
}

void APIServlet::removeTableField(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (req->method() != http::HTTPMessage::kHTTPMethod::M_POST) {
    res->setStatus(http::kStatusMethodNotAllowed);
    res->addHeader("Content-Type", "text/plain; charset=utf-8");
    res->addBody("expected POST request");
    return;
  }

  auto dbctx = session->getDatabaseContext();
  auto jreq = json::parseJSON(req->body());

  /* database */
  auto database = json::objectGetString(jreq, "database");
  if (!database.isEmpty()) {
    auto auth_rc = dbctx->client_auth->changeNamespace(session, database.get());
    if (!auth_rc.isSuccess()) {
      res->setStatus(http::kStatusForbidden);
      res->addHeader("Content-Type", "text/plain; charset=utf-8");
      res->addBody(auth_rc.message());
      return;
    }
  }

  if (session->getEffectiveNamespace().empty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addHeader("Content-Type", "text/plain; charset=utf-8");
    res->addBody("no database selected");
    return;
  }

  auto table_name = json::objectGetString(jreq, "table");
  if (table_name.isEmpty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing field: table");
    return;
  }

  auto field_name = json::objectGetString(jreq, "field_name");
  if (field_name.isEmpty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing field: field_name");
    return;
  }

  Vector<TableService::AlterTableOperation> operations;
  TableService::AlterTableOperation operation;
  operation.optype = TableService::AlterTableOperationType::OP_REMOVE_COLUMN;
  operation.field_name = field_name.get();
  operations.emplace_back(operation);

  auto rc = dbctx->table_service->alterTable(
      session->getEffectiveNamespace(),
      table_name.get(),
      operations);

  if (!rc.isSuccess()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody(StringUtil::format("error: $0", rc.message()));
    return;
  }

  res->setStatus(http::kStatusCreated);
  res->addBody("ok");
  return;
}

void APIServlet::dropTable(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (req->method() != http::HTTPMessage::kHTTPMethod::M_POST) {
    res->setStatus(http::kStatusMethodNotAllowed);
    res->addHeader("Content-Type", "text/plain; charset=utf-8");
    res->addBody("expected POST request");
    return;
  }

  auto dbctx = session->getDatabaseContext();
  auto jreq = json::parseJSON(req->body());

  /* database */
  auto database = json::objectGetString(jreq, "database");
  if (!database.isEmpty()) {
    auto auth_rc = dbctx->client_auth->changeNamespace(session, database.get());
    if (!auth_rc.isSuccess()) {
      res->setStatus(http::kStatusForbidden);
      res->addHeader("Content-Type", "text/plain; charset=utf-8");
      res->addBody(auth_rc.message());
      return;
    }
  }

  if (session->getEffectiveNamespace().empty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addHeader("Content-Type", "text/plain; charset=utf-8");
    res->addBody("no database selected");
    return;
  }

  auto table_name = json::objectGetString(jreq, "table");
  if (table_name.isEmpty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing field: table");
    return;
  }


  auto rc = dbctx->table_service->dropTable(
      session->getEffectiveNamespace(),
      table_name.get());

  if (!rc.isSuccess()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody(StringUtil::format("error: $0", rc.message()));
    return;
  }

  res->setStatus(http::kStatusCreated);
  res->addBody("ok");
  return;
}

void APIServlet::insertIntoTable(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (req->method() != http::HTTPMessage::kHTTPMethod::M_POST) {
    res->setStatus(http::kStatusMethodNotAllowed);
    res->addHeader("Content-Type", "text/plain; charset=utf-8");
    res->addBody("expected POST request");
    return;
  }

  auto dbctx = session->getDatabaseContext();
  auto jreq = json::parseJSON(req->body());

  auto ncols = json::arrayLength(jreq.begin(), jreq.end());
  for (size_t i = 0; i < ncols; ++i) {
    auto jrow = json::arrayLookup(jreq.begin(), jreq.end(), i); // O(N^2) but who cares...

    auto table = json::objectGetString(jrow, jreq.end(), "table");
    if (table.isEmpty()) {
      RAISE(kRuntimeError, "missing field: table");
    }

    String insert_database = session->getEffectiveNamespace();

    auto hdrval = req->getHeader("X-Z1-Namespace");
    if (!hdrval.empty()) {
      insert_database = hdrval;
    }

    auto database = json::objectGetString(jrow, jreq.end(), "database");
    if (!database.isEmpty()) {
      insert_database = database.get();
    }

    if (insert_database.empty()) {
      RAISE(kRuntimeError, "missing field: database");
    }

    auto tc = dbctx->table_service->tableConfig(insert_database, table.get());
    if (tc.isEmpty()) {
      res->setStatus(http::kStatusForbidden);
      return;
    }

    if (!tc.get().config().allow_public_insert() &&
        insert_database != session->getEffectiveNamespace()) {
      auto rc = dbctx->client_auth->changeNamespace(session, insert_database);
      if (!rc.isSuccess()) {
        res->setStatus(http::kStatusForbidden);
        return;
      }
    }

    auto data = json::objectLookup(jrow, jreq.end(), "data");
    if (data == jreq.end()) {
      RAISE(kRuntimeError, "missing field: data");
    }

    auto rc = ReturnCode::success();
    if (data->type == json::JSON_STRING) {
      try {
        auto data_parsed = json::parseJSON(data->data);
        rc = dbctx->table_service->insertRecord(
            insert_database,
            table.get(),
            data_parsed.begin(),
            data_parsed.end());
      } catch (const std::exception& e) {
        rc = ReturnCode::exception(e);
      }
    } else {
      rc = dbctx->table_service->insertRecord(
          insert_database,
          table.get(),
          data,
          data + data->size);
    }

    if (rc.isSuccess()) {
      res->setStatus(http::kStatusCreated);
    } else {
      res->setStatus(http::kStatusInternalServerError);
      res->addBody("ERROR: " + rc.getMessage());
      return;
    }
  }

  res->setStatus(http::kStatusCreated);
}

void APIServlet::getAuthInfo(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  Buffer buf;

  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  json.beginObject();
  json.addObjectEntry("valid");
  json.addTrue();
  json.addComma();
  json.addObjectEntry("namespace");
  json.addString(session->getEffectiveNamespace());
  json.addComma();
  json.addObjectEntry("user_id");
  json.addString(session->getUserID());
  json.endObject();

  res->addBody(buf);
  res->setStatus(http::kStatusOK);
}

void APIServlet::executeSQL(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
  try {
    std::string format = "json";
    std::string query;
    std::string database;
    if (req->method() == http::HTTPMessage::kHTTPMethod::M_GET) {
      URI uri(req->uri());
      URI::ParamList params = uri.queryParams();
      URI::parseQueryString(req->body().toString(), &params);
      URI::getParam(params, "format", &format);
      URI::getParam(params, "database", &database);

      if (!URI::getParam(params, "query", &query)) {
        res->setStatus(http::kStatusBadRequest);
        res->addBody("missing ?query=... parameter");
        res_stream->writeResponse(*res);
        return;
      }

    } else {
      auto jreq = json::parseJSON(req->body());
      auto format_opt = json::objectGetString(jreq, "format");
      if (!format_opt.isEmpty()) {
        format = format_opt.get();
      }

      auto query_opt = json::objectGetString(jreq, "query");
      if (query_opt.isEmpty()) {
        res->setStatus(http::kStatusBadRequest);
        res->addBody("missing field: query");
        res_stream->writeResponse(*res);
        return;
      } else {
        query = query_opt.get();
      }

      auto db_opt = json::objectGetString(jreq, "database");
      if (!db_opt.isEmpty()) {
        database = db_opt.get();
      }
    }

    if (format == "binary") {
      executeSQL_BINARY(query, database, session, res, res_stream);
    } else if (format == "json") {
      executeSQL_JSON(query, database, session, res, res_stream);
    } else if (format == "json_sse") {
      executeSQL_JSONSSE(query, database, session, res, res_stream);
    } else {
      res->setStatus(http::kStatusBadRequest);
      res->addBody("invalid format: " + format);
      res_stream->writeResponse(*res);
    }
  } catch (const StandardException& e) {
    logError("evqld", e, "Uncaught SQL error");
    res->setStatus(http::kStatusBadRequest);
    res->addBody("invalid request");
    res_stream->writeResponse(*res);
  }
}

void APIServlet::executeSQL_ASCII(
    const std::string& query,
    const std::string& database,
    Session* session,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
//  String query;
//  if (!URI::getParam(params, "query", &query)) {
//    res->setStatus(http::kStatusBadRequest);
//    res->addBody("missing ?query=... parameter");
//    res_stream->writeResponse(*res);
//    return;
//  }
//
//  try {
//    auto txn = dbctx->sql_runtime->newTransaction();
//    auto estrat = app_->getExecutionStrategy(session->getEffectiveNamespace());
//    txn->setTableProvider(estrat->tableProvider());
//    auto qplan = dbctx->sql_runtime->buildQueryPlan(txn.get(), query, estrat);
//
//    ASCIICodec ascii_codec(qplan.get());
//    qplan->execute();
//
//    Buffer result;
//    ascii_codec.printResults(BufferOutputStream::fromBuffer(&result));
//
//    res->setStatus(http::kStatusOK);
//    res->addHeader("Content-Type", "text/plain; charset=utf-8");
//    res->addBody(result);
//    res_stream->writeResponse(*res);
//  } catch (const StandardException& e) {
//    res->setStatus(http::kStatusInternalServerError);
//    res->addHeader("Content-Type", "text/plain; charset=utf-8");
//    res->addBody(StringUtil::format("error: $0", e.what()));
//    res_stream->writeResponse(*res);
//  }
}

void APIServlet::executeSQL_BINARY(
    const std::string& query,
    const std::string& database,
    Session* session,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
  auto dbctx = session->getDatabaseContext();

  res->setStatus(http::kStatusOK);
  res->setHeader("Connection", "close");
  res->setHeader("Content-Type", "application/octet-stream");
  res->setHeader("Cache-Control", "no-cache");
  res->setHeader("Access-Control-Allow-Origin", "*");
  res_stream->startResponse(*res);

  {
    auto write_cb = [res_stream] (const void* data, size_t size) {
      res_stream->writeBodyChunk(data, size);
    };

    csql::BinaryResultFormat result_format(write_cb, true);

    if (!database.empty()) {
      auto rc = dbctx->client_auth->changeNamespace(session, database);
      if (!rc.isSuccess()) {
        result_format.sendError(rc.message());
        res_stream->finishResponse();
        return;
      }
    }

    if (session->getEffectiveNamespace().empty()) {
      result_format.sendError("No database selected");
      res_stream->finishResponse();
      return;
    }

    try {
      auto txn = dbctx->sql_service->startTransaction(session);
      auto qplan = dbctx->sql_runtime->buildQueryPlan(txn.get(), query);
      qplan->setProgressCallback([&result_format, &qplan] () {
        result_format.sendProgress(qplan->getProgress());
      });

      result_format.sendResults(qplan.get());
    } catch (const StandardException& e) {
      result_format.sendError(e.what());
    }
  }

  res_stream->finishResponse();
}

void APIServlet::executeSQL_JSON(
    const std::string& query,
    const std::string& database,
    Session* session,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
  auto dbctx = session->getDatabaseContext();

  if (!database.empty()) {
    auto rc = dbctx->client_auth->changeNamespace(session, database);
    if (!rc.isSuccess()) {
      Buffer buf;
      json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
      json.beginObject();
      json.addObjectEntry("error");
      json.addString(rc.message());
      json.endObject();

      res->setStatus(http::kStatusInternalServerError);
      res->addHeader("Content-Type", "application/json; charset=utf-8");
      res->addBody(buf);
      res_stream->writeResponse(*res);
      return;
    }
  }

  if (session->getEffectiveNamespace().empty()) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("error");
    json.addString("No database selected");
    json.endObject();

    res->setStatus(http::kStatusInternalServerError);
    res->addHeader("Content-Type", "application/json; charset=utf-8");
    res->addBody(buf);
    res_stream->writeResponse(*res);
    return;
  }

  try {
    auto txn = dbctx->sql_service->startTransaction(session);
    auto qplan = dbctx->sql_runtime->buildQueryPlan(txn.get(), query);

    Buffer result;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&result));
    JSONCodec json_codec(&json);
    json.beginObject();
    json.addObjectEntry("results");
    json.beginArray();

    for (size_t i = 0; i < qplan->numStatements(); ++i) {
      if (i > 0) {
        json.addComma();
      }

      auto result_columns = qplan->getStatementgetResultColumns(i);
      auto result_cursor = qplan->execute(i);
      json_codec.printResultTable(result_columns, result_cursor.get());
    }

    json.endArray();
    json.endObject();

    res->setStatus(http::kStatusOK);
    res->addHeader("Content-Type", "application/json; charset=utf-8");
    res->addBody(result);
    res_stream->writeResponse(*res);
  } catch (const StandardException& e) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("error");
    json.addString(e.what());
    json.endObject();

    res->setStatus(http::kStatusInternalServerError);
    res->addHeader("Content-Type", "application/json; charset=utf-8");
    res->addBody(buf);
    res_stream->writeResponse(*res);
  }
}

void APIServlet::executeSQL_JSONSSE(
    const std::string& query,
    const std::string& database,
    Session* session,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
  auto dbctx = session->getDatabaseContext();

  auto sse_stream = mkRef(new http::HTTPSSEStream(res, res_stream));
  sse_stream->start();

  if (!database.empty()) {
    auto rc = dbctx->client_auth->changeNamespace(session, database);
    if (!rc.isSuccess()) {
      Buffer buf;
      json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
      json.beginObject();
      json.addObjectEntry("error");
      json.addString(rc.message());
      json.endObject();

      sse_stream->sendEvent(buf, Some(String("query_error")));
      sse_stream->finish();
      return;
    }
  }

  if (session->getEffectiveNamespace().empty()) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("error");
    json.addString("No database selected");
    json.endObject();

    sse_stream->sendEvent(buf, Some(String("query_error")));
    sse_stream->finish();
    return;
  }

  try {
    auto txn = dbctx->sql_service->startTransaction(session);
    auto qplan = dbctx->sql_runtime->buildQueryPlan(txn.get(), query);

    JSONSSECodec json_sse_codec(sse_stream);
    qplan->setProgressCallback([&json_sse_codec, &qplan] () {
      json_sse_codec.sendProgress(qplan->getProgress());
    });

    Vector<csql::ResultList> results;
    for (size_t i = 0; i < qplan->numStatements(); ++i) {
      results.emplace_back(qplan->getStatementgetResultColumns(i));
      qplan->execute(i, &results.back());
    }

   json_sse_codec.sendResults(results); 
  } catch (const StandardException& e) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("error");
    json.addString(e.what());
    json.endObject();

    sse_stream->sendEvent(buf, Some(String("query_error")));
  }

  sse_stream->finish();
}

} // namespace eventql
