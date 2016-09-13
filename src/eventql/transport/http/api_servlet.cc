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
    res.addHeader("Content-Type", "text/html; charset=utf-8");
    res.addBody(Assets::getAsset("eventql/webui/500.html"));
    res_stream->writeResponse(res);
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

  auto internal_auth_rc = dbctx->internal_auth->verifyRequest(session, req);
  auto auth_rc = internal_auth_rc;
  if (!auth_rc.isSuccess()) {
    auth_rc = HTTPAuth::authenticateRequest(
        session,
        dbctx->client_auth,
        req);
  }

  if (uri.path() == "/api/v1/tables/insert") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, session, &req, &res] {
      insertIntoTable(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (!auth_rc.isSuccess()) {
    req_stream->readBody();

    if (uri.path() == "/api/v1/sql") {
      util::BinaryMessageWriter writer;
      res.setStatus(http::kStatusOK);
      writer.appendUInt8(0xf4);
      writer.appendLenencString(auth_rc.message());
      writer.appendUInt8(0xff);
      res.addBody(writer.data(), writer.size());
      res_stream->writeResponse(res);
    } else {
      res.setStatus(http::kStatusUnauthorized);
      res.addHeader("WWW-Authenticate", "Token");
      res.addHeader("Content-Type", "text/plain; charset=utf-8");
      res.addBody(auth_rc.message());
      res_stream->writeResponse(res);
      return;
    }
  }

  /* SQL */
  if (uri.path() == "/api/v1/sql" ||
      uri.path() == "/api/v1/sql_stream") {
    req_stream->readBody();
    executeSQL(session, &req, &res, res_stream);
    return;
  }

  if (uri.path() == "/api/v1/auth/info") {
    req_stream->readBody();
    getAuthInfo(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (StringUtil::beginsWith(uri.path(), "/api/v1/mapreduce")) {
    mapreduce_api_.handle(session, req_stream, res_stream);
    return;
  }

  if (session->getEffectiveNamespace().empty()) {
    res.setStatus(http::kStatusUnauthorized);
    res.addHeader("WWW-Authenticate", "Token");
    res.addHeader("Content-Type", "text/plain; charset=utf-8");
    res.addBody("unauthorized");
    res_stream->writeResponse(res);
    return;
  }

  /* TABLES */
  if (uri.path() == "/api/v1/tables") {
    req_stream->readBody();
    listTables(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/create_table") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      createTable(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/add_field") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      addTableField(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/remove_field") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      removeTableField(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/add_tag") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      addTableTag(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/remove_tag") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      removeTableTag(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  static const String kTablesPathPrefix = "/api/v1/tables/";
  if (StringUtil::beginsWith(uri.path(), kTablesPathPrefix)) {
    req_stream->readBody();
    fetchTableDefinition(
        session,
        uri.path().substr(kTablesPathPrefix.size()),
        &req,
        &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/sql/execute_qtree") {
    req_stream->readBody();
    executeQTree(session, &req, &res, res_stream);
    return;
  }

  if (uri.path() == "/api/v1/sql/scan_partition") {
    req_stream->readBody();
    //executeSQLScanPartition(session, &req, &res, res_stream);
    return;
  }

  res.setStatus(http::kStatusNotFound);
  res.addHeader("Content-Type", "text/html; charset=utf-8");
  res.addBody(Assets::getAsset("eventql/webui/404.html"));
  res_stream->writeResponse(res);
}

void APIServlet::listTables(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto dbctx = session->getDatabaseContext();

  URI uri(req->uri());
  const auto& params = uri.queryParams();

  /* param tag */
  String tag_filter;
  URI::getParam(params, "tag", &tag_filter);

  if (tag_filter == "all") {
    tag_filter.clear();
  }

  /* param sort_fn */
  String order_filter;
  URI::getParam(params, "order", &order_filter);

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));

  json.beginObject();
  json.addObjectEntry("tables");
  json.beginArray();

  size_t ntable = 0;

  auto writeTableJSON = [&json, &ntable, &tag_filter] (const TSDBTableInfo& table) {

    Set<String> tags;
    for (const auto& tag : table.config.tags()) {
      tags.insert(tag);
    }

    if (!tag_filter.empty()) {
      if (tags.count(tag_filter) == 0) {
        return;
      }
    }

    if (++ntable > 1) {
      json.addComma();
    }

    json.beginObject();

    json.addObjectEntry("name");
    json.addString(table.table_name);
    json.addComma();

    json.addObjectEntry("tags");
    json::toJSON(tags, &json);

    json.endObject();

  };

  if (order_filter == "desc") {
    dbctx->table_service->listTablesReverse(session->getEffectiveNamespace(), writeTableJSON);
  } else {
    dbctx->table_service->listTables(session->getEffectiveNamespace(), writeTableJSON);
  }

  json.endArray();
  json.endObject();

  res->setStatus(http::kStatusOK);
  res->setHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(buf);
}

void APIServlet::fetchTableDefinition(
    Session* session,
    const String& table_name,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto dbctx = session->getDatabaseContext();

  auto table = dbctx->partition_map->findTable(session->getEffectiveNamespace(), table_name);
  if (table.isEmpty()) {
    res->setStatus(http::kStatusNotFound);
    res->addBody("table not found");
    return;
  }

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  auto schema = table.get()->schema();
  schema->toJSON(&json);

  res->setStatus(http::kStatusOK);
  res->setHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(buf);
}

void APIServlet::createTable(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto dbctx = session->getDatabaseContext();

  auto jreq = json::parseJSON(req->body());

  auto table_name = json::objectGetString(jreq, "table_name");
  if (table_name.isEmpty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing field: table_name");
    return;
  }

  auto jschema = json::objectLookup(jreq, "schema");
  if (jschema == jreq.end()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing field: schema");
    return;
  }

  auto num_shards = json::objectGetUInt64(jreq, "num_shards");

  String table_type = "static";
  auto jtable_type = json::objectGetString(jreq, "table_type");
  if (!jtable_type.isEmpty()) {
    table_type = jtable_type.get();
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
        res->addBody("invalid field: primary_key");
        return;
      }

      properties.emplace_back(prop_key.get(), prop_value.get());
    }
  }

  try {
    msg::MessageSchema schema(nullptr);
    schema.fromJSON(jschema, jreq.end());

    auto rc = dbctx->table_service->createTable(
        session->getEffectiveNamespace(),
        table_name.get(),
        schema,
        primary_key,
        properties);

    if (!rc.isSuccess()) {
      RAISE(kRuntimeError, rc.message());
    }

    res->setStatus(http::kStatusCreated);
  } catch (const StandardException& e) {
    logError("analyticsd", e, "error");
    res->setStatus(http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0", e.what()));
    return;
  }

  res->setStatus(http::kStatusCreated);
}

void APIServlet::addTableField(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto dbctx = session->getDatabaseContext();

  URI uri(req->uri());
  const auto& params = uri.queryParams();

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto table_opt = dbctx->partition_map->findTable(session->getEffectiveNamespace(), table_name);
  if (table_opt.isEmpty()) {
    res->setStatus(http::kStatusNotFound);
    res->addBody("table not found");
    return;
  }
  const auto& table = table_opt.get();

  String field_name;
  if (!URI::getParam(params, "field_name", &field_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing &field_name=... parameter");
    return;
  }

  String field_type_str;
  if (!URI::getParam(params, "field_type", &field_type_str)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing &field_name=... parameter");
    return;
  }

  String repeated_param;
  URI::getParam(params, "repeated", &repeated_param);
  auto repeated = Human::parseBoolean(repeated_param);

  String optional_param;
  URI::getParam(params, "optional", &optional_param);
  auto optional = Human::parseBoolean(optional_param);

  auto td = table->config();
  auto schema = msg::MessageSchema::decode(td.config().schema());

  uint32_t next_field_id;
  if (td.has_next_field_id()) {
    next_field_id = td.next_field_id();
  } else {
    next_field_id = schema->maxFieldId() + 1;
  }

  auto cur_schema = schema;
  auto field = field_name;

  while (StringUtil::includes(field, ".")) {
    auto prefix_len = field.find(".");
    auto prefix = field.substr(0, prefix_len);

    field = field.substr(prefix_len + 1);
    if (!cur_schema->hasField(prefix)) {
      res->setStatus(http::kStatusNotFound);
      res->addBody(StringUtil::format("field $0 not found", prefix));
      return;
    }

    auto parent_field_id = cur_schema->fieldId(prefix);
    auto parent_field_type = cur_schema->fieldType(parent_field_id);
    if (parent_field_type != msg::FieldType::OBJECT) {
      res->setStatus(http::kStatusBadRequest);
      res->addBody(StringUtil::format(
        "can't add field to a field of type $0",
        fieldTypeToString(parent_field_type)));
      return;
    }

    cur_schema = cur_schema->fieldSchema(parent_field_id);
  }

  auto field_type = msg::fieldTypeFromString(field_type_str);
  if (field_type == msg::FieldType::OBJECT) {
    cur_schema->addField(
          msg::MessageSchemaField::mkObjectField(
              next_field_id,
              field,
              repeated.isEmpty() ? false : repeated.get(),
              optional.isEmpty() ? false : optional.get(),
              mkRef(new msg::MessageSchema(nullptr))));


  } else {
    cur_schema->addField(
          msg::MessageSchemaField(
              next_field_id,
              field,
              field_type,
              0,
              repeated.isEmpty() ? false : repeated.get(),
              optional.isEmpty() ? false : optional.get()));
  }


  td.set_next_field_id(next_field_id + 1);
  td.mutable_config()->set_schema(schema->encode().toString());

  dbctx->config_directory->updateTableConfig(td);
  res->setStatus(http::kStatusCreated);
  res->addBody("ok");
  return;
}

void APIServlet::removeTableField(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto dbctx = session->getDatabaseContext();

  URI uri(req->uri());
  const auto& params = uri.queryParams();

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto table_opt = dbctx->partition_map->findTable(session->getEffectiveNamespace(), table_name);
  if (table_opt.isEmpty()) {
    res->setStatus(http::kStatusNotFound);
    res->addBody("table not found");
    return;
  }
  const auto& table = table_opt.get();

  String field_name;
  if (!URI::getParam(params, "field_name", &field_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing &field_name=... parameter");
    return;
  }

  auto td = table->config();
  auto schema = msg::MessageSchema::decode(td.config().schema());
  auto cur_schema = schema;
  auto field = field_name;

  while (StringUtil::includes(field, ".")) {
    auto prefix_len = field.find(".");
    auto prefix = field.substr(0, prefix_len);

    field = field.substr(prefix_len + 1);

    if (!cur_schema->hasField(prefix)) {
      res->setStatus(http::kStatusNotFound);
      res->addBody("field not found");
      return;
    }
    cur_schema = cur_schema->fieldSchema(cur_schema->fieldId(prefix));
  }

  if (!cur_schema->hasField(field)) {
    res->setStatus(http::kStatusNotFound);
    res->addBody("field not found");
    return;
  }

  if (!td.has_next_field_id()) {
    td.set_next_field_id(schema->maxFieldId() + 1);
  }

  cur_schema->removeField(cur_schema->fieldId(field));
  td.mutable_config()->set_schema(schema->encode().toString());

  dbctx->config_directory->updateTableConfig(td);
  res->setStatus(http::kStatusCreated);
  res->addBody("ok");
  return;
}

void APIServlet::addTableTag(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto dbctx = session->getDatabaseContext();

  URI uri(req->uri());
  const auto& params = uri.queryParams();

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto table_opt = dbctx->partition_map->findTable(session->getEffectiveNamespace(), table_name);
  if (table_opt.isEmpty()) {
    res->setStatus(http::kStatusNotFound);
    res->addBody("table not found");
    return;
  }
  const auto& table = table_opt.get();

  String tag;
  if (!URI::getParam(params, "tag", &tag)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing &tag=... parameter");
    return;
  }

  auto td = table->config();
  td.add_tags(tag);

  dbctx->config_directory->updateTableConfig(td);
  res->setStatus(http::kStatusCreated);
  res->addBody("ok");
  return;
}

void APIServlet::removeTableTag(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto dbctx = session->getDatabaseContext();

  URI uri(req->uri());
  const auto& params = uri.queryParams();

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  String tag;
  if (!URI::getParam(params, "tag", &tag)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?tag=... parameter");
    return;
  }

  auto table_opt = dbctx->partition_map->findTable(
      session->getEffectiveNamespace(),
      table_name);

  if (table_opt.isEmpty()) {
    res->setStatus(http::kStatusNotFound);
    res->addBody("table not found");
    return;
  }

  auto table = table_opt.get();
  auto td = table->config();
  auto tags = td.mutable_tags();


  for (size_t i = tags->size() - 1; ; --i) {
    if (tags->Get(i) == tag) {
      tags->DeleteSubrange(i, 1);
    }

    if (i == 0) {
      break;
    }

  }

  dbctx->config_directory->updateTableConfig(td);
  res->setStatus(http::kStatusCreated);
  res->addBody("ok");
  return;
}


void APIServlet::insertIntoTable(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
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

    if (data->type == json::JSON_STRING) {
      auto data_parsed = json::parseJSON(data->data);
      dbctx->table_service->insertRecord(
          insert_database,
          table.get(),
          data_parsed.begin(),
          data_parsed.end());
    } else {
      dbctx->table_service->insertRecord(
          insert_database,
          table.get(),
          data,
          data + data->size);
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
    URI uri(req->uri());
    URI::ParamList params = uri.queryParams();
    URI::parseQueryString(req->body().toString(), &params);

    String format;
    URI::getParam(params, "format", &format);
    if (format.empty()) {
      format = "json";
    }

    if (format == "binary") {
      executeSQL_BINARY(params, session, req, res, res_stream);
    } else if (format == "json") {
      executeSQL_JSON(params, session, req, res, res_stream);
    } else if (format == "json_sse") {
      executeSQL_JSONSSE(params, session, req, res, res_stream);
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
    const URI::ParamList& params,
    Session* session,
    const http::HTTPRequest* req,
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
    const URI::ParamList& params,
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
  auto dbctx = session->getDatabaseContext();

  String query;
  if (!URI::getParam(params, "query", &query)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?query=... parameter");
    res_stream->writeResponse(*res);
    return;
  }

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

    String database;
    if (URI::getParam(params, "database", &database) && !database.empty()) {
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
    const URI::ParamList& params,
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
  auto dbctx = session->getDatabaseContext();

  String query;
  if (!URI::getParam(params, "query", &query)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?query=... parameter");
    res_stream->writeResponse(*res);
    return;
  }

  String database;
  if (URI::getParam(params, "database", &database) && !database.empty()) {
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
    const URI::ParamList& params,
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
  auto dbctx = session->getDatabaseContext();

  String query;
  if (!URI::getParam(params, "query", &query)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?query=... parameter");
    res_stream->writeResponse(*res);
    return;
  }

  auto sse_stream = mkRef(new http::HTTPSSEStream(res, res_stream));
  sse_stream->start();

  String database;
  if (URI::getParam(params, "database", &database) && !database.empty()) {
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

void APIServlet::executeQTree(
    Session* session,
    const http::HTTPRequest* req,
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
    csql::BinaryResultFormat result_format(
        [res_stream] (const void* data, size_t size) {
      res_stream->writeBodyChunk(data, size);
    });

    //String database;
    //if (URI::getParam(params, "database", &database) && !database.empty()) {
    //  auto rc = dbctx->client_auth->changeNamespace(session, database);
    //  if (!rc.isSuccess()) {
    //    result_format.sendError(rc.message());
    //    res_stream->finishResponse();
    //    return;
    //  }
    //}

    if (session->getEffectiveNamespace().empty()) {
      result_format.sendError("No database selected");
      res_stream->finishResponse();
      return;
    }

    try {
      auto txn = dbctx->sql_service->startTransaction(session);

      csql::QueryTreeCoder coder(txn.get());
      auto req_body_is = BufferInputStream::fromBuffer(&req->body());
      auto qtree = coder.decode(req_body_is.get());
      auto qplan = dbctx->sql_runtime->buildQueryPlan(txn.get(), { qtree });

      result_format.sendResults(qplan.get());
    } catch (const StandardException& e) {
      logError("evql", "SQL Error: $0", e.what());
      result_format.sendError(e.what());
    }
  }

  res_stream->finishResponse();
}

} // namespace eventql
