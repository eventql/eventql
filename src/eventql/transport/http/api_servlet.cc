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
#include "eventql/db/TableConfig.pb.h"
#include "eventql/server/sql/codec/json_codec.h"
#include "eventql/server/sql/codec/json_sse_codec.h"
#include "eventql/server/sql/codec/binary_codec.h"
#include "eventql/db/TimeWindowPartitioner.h"
#include "eventql/db/FixedShardPartitioner.h"
#include "eventql/transport/http/http_auth.h"
#include <eventql/io/cstable/cstable_writer.h>
#include <eventql/io/cstable/RecordShredder.h>

namespace eventql {

AnalyticsServlet::AnalyticsServlet(
    const String& cachedir,
    InternalAuth* auth,
    ClientAuth* client_auth,
    InternalAuth* internal_auth,
    csql::Runtime* sql,
    eventql::TableService* tsdb,
    ConfigDirectory* customer_dir,
    PartitionMap* pmap,
    SQLService* sql_service,
    LogfileService* logfile_service,
    MapReduceService* mapreduce_service,
    TableService* table_service) :
    cachedir_(cachedir),
    auth_(auth),
    client_auth_(client_auth),
    internal_auth_(internal_auth),
    sql_(sql),
    tsdb_(tsdb),
    customer_dir_(customer_dir),
    pmap_(pmap),
    sql_service_(sql_service),
    logfile_service_(logfile_service),
    mapreduce_service_(mapreduce_service),
    table_service_(table_service),
    logfile_api_(logfile_service_, customer_dir, cachedir),
    mapreduce_api_(mapreduce_service_, customer_dir, cachedir) {}

void AnalyticsServlet::handleHTTPRequest(
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

void AnalyticsServlet::handle(
    RefPtr<http::HTTPRequestStream> req_stream,
    RefPtr<http::HTTPResponseStream> res_stream) {
  const auto& req = req_stream->request();
  URI uri(req.uri());

  logDebug("eventql", "HTTP Request: $0 $1", req.method(), req.uri());

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

  ScopedPtr<Session> session(new Session());
  auto internal_auth_rc = internal_auth_->verifyRequest(session.get(), req);
  auto auth_rc = internal_auth_rc;
  if (!auth_rc.isSuccess()) {
    auth_rc = HTTPAuth::authenticateRequest(
        session.get(),
        client_auth_,
        req);
  }

  if (uri.path() == "/api/v1/tables/insert") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      insertIntoTable(session.get(), &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (!auth_rc.isSuccess()) {
    req_stream->readBody();

    logError("evqld", "auth error: $0 -- $1", auth_rc.message(), internal_auth_rc.message());

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
    executeSQL(session.get(), &req, &res, res_stream);
    return;
  }

  if (uri.path() == "/api/v1/auth/info") {
    req_stream->readBody();
    getAuthInfo(session.get(), &req, &res);
    res_stream->writeResponse(res);
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

  if (StringUtil::beginsWith(uri.path(), "/api/v1/logfiles")) {
    logfile_api_.handle(session.get(), req_stream, res_stream);
    return;
  }

  if (StringUtil::beginsWith(uri.path(), "/api/v1/mapreduce")) {
    mapreduce_api_.handle(session.get(), req_stream, res_stream);
    return;
  }

  /* TABLES */
  if (uri.path() == "/api/v1/tables") {
    req_stream->readBody();
    listTables(session.get(), &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/create_table") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      createTable(session.get(), &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/upload_table") {
    expectHTTPPost(req);
    uploadTable(uri, session.get(), req_stream.get(), &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/add_field") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      addTableField(session.get(), &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/remove_field") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      removeTableField(session.get(), &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/add_tag") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      addTableTag(session.get(), &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/remove_tag") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      removeTableTag(session.get(), &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  static const String kTablesPathPrefix = "/api/v1/tables/";
  if (StringUtil::beginsWith(uri.path(), kTablesPathPrefix)) {
    req_stream->readBody();
    fetchTableDefinition(
        session.get(),
        uri.path().substr(kTablesPathPrefix.size()),
        &req,
        &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/sql/execute_qtree") {
    req_stream->readBody();
    executeQTree(session.get(), &req, &res, res_stream);
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

void AnalyticsServlet::listTables(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
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
    table_service_->listTablesReverse(session->getEffectiveNamespace(), writeTableJSON);
  } else {
    table_service_->listTables(session->getEffectiveNamespace(), writeTableJSON);
  }

  json.endArray();
  json.endObject();

  res->setStatus(http::kStatusOK);
  res->setHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(buf);
}

void AnalyticsServlet::fetchTableDefinition(
    Session* session,
    const String& table_name,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto table = pmap_->findTable(session->getEffectiveNamespace(), table_name);
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

void AnalyticsServlet::createTable(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
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

  auto update_param = json::objectGetBool(jreq, "update");
  auto force = !update_param.isEmpty() && update_param.get();

  try {
    msg::MessageSchema schema(nullptr);
    schema.fromJSON(jschema, jreq.end());

    // legacy static tables
    if (table_type == "static" || table_type == "static_fixed") {
      TableDefinition td;
      if (force) {
        try {
          auto old_td = customer_dir_->getTableConfig(
              session->getEffectiveNamespace(),
              table_name.get());

          td.set_version(old_td.version());
        } catch (const std::exception& e) {
          // ignore
        }
      }

      td.set_customer(session->getEffectiveNamespace());
      td.set_table_name(table_name.get());

      auto tblcfg = td.mutable_config();
      tblcfg->set_schema(schema.encode().toString());
      tblcfg->set_num_shards(num_shards.isEmpty() ? 1 : num_shards.get());
      tblcfg->set_partitioner(eventql::TBL_PARTITION_FIXED);
      tblcfg->set_storage(eventql::TBL_STORAGE_STATIC);
      customer_dir_->updateTableConfig(td);
    } else {
      auto rc = table_service_->createTable(
          session->getEffectiveNamespace(),
          table_name.get(),
          schema,
          { "time" }); // FIXME

      if (!rc.isSuccess()) {
        RAISE(kRuntimeError, rc.message());
      }
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

void AnalyticsServlet::addTableField(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {

  URI uri(req->uri());
  const auto& params = uri.queryParams();

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto table_opt = pmap_->findTable(session->getEffectiveNamespace(), table_name);
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

  customer_dir_->updateTableConfig(td);
  res->setStatus(http::kStatusCreated);
  res->addBody("ok");
  return;
}

void AnalyticsServlet::removeTableField(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {

  URI uri(req->uri());
  const auto& params = uri.queryParams();

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto table_opt = pmap_->findTable(session->getEffectiveNamespace(), table_name);
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

  customer_dir_->updateTableConfig(td);
  res->setStatus(http::kStatusCreated);
  res->addBody("ok");
  return;
}

void AnalyticsServlet::addTableTag(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {

  URI uri(req->uri());
  const auto& params = uri.queryParams();

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table=... parameter");
    return;
  }

  auto table_opt = pmap_->findTable(session->getEffectiveNamespace(), table_name);
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

  customer_dir_->updateTableConfig(td);
  res->setStatus(http::kStatusCreated);
  res->addBody("ok");
  return;
}

void AnalyticsServlet::removeTableTag(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {

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

  auto table_opt = pmap_->findTable(session->getEffectiveNamespace(), table_name);
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

  customer_dir_->updateTableConfig(td);
  res->setStatus(http::kStatusCreated);
  res->addBody("ok");
  return;
}


void AnalyticsServlet::insertIntoTable(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
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

    auto tc = tsdb_->tableConfig(insert_database, table.get());
    if (tc.isEmpty()) {
      res->setStatus(http::kStatusForbidden);
      return;
    }

    if (!tc.get().config().allow_public_insert() &&
        insert_database != session->getEffectiveNamespace()) {
      auto rc = client_auth_->changeNamespace(session, insert_database);
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
      tsdb_->insertRecord(
          insert_database,
          table.get(),
          data_parsed.begin(),
          data_parsed.end());
    } else {
      tsdb_->insertRecord(
          insert_database,
          table.get(),
          data,
          data + data->size);
    }
  }

  res->setStatus(http::kStatusCreated);
}

void AnalyticsServlet::uploadTable(
    const URI& uri,
    Session* session,
    http::HTTPRequestStream* req_stream,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();

  String table_name;
  if (!URI::getParam(params, "table", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?table=... parameter");
    return;
  }

  String shard_str;
  if (!URI::getParam(params, "shard", &shard_str)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?shard=... parameter");
    return;
  }

  auto shard = std::stoull(shard_str);
  auto schema = tsdb_->tableSchema(session->getEffectiveNamespace(), table_name);
  if (schema.isEmpty()) {
    res->setStatus(http::kStatusNotFound);
    res->addBody("error: table not found");
    return;
  }

  auto tmpfile_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("upload_$0.tmp", Random::singleton()->hex128()));

  auto partition_key = eventql::FixedShardPartitioner::partitionKeyFor(
      table_name,
      shard);

  try {
    auto cstable = cstable::CSTableWriter::createFile(
        tmpfile_path + ".cst",
        cstable::BinaryFormatVersion::v0_1_0,
        cstable::TableSchema::fromProtobuf(*schema.get()));

    cstable::RecordShredder shredder(cstable.get());

    {
      auto tmpfile = File::openFile(
          tmpfile_path,
          File::O_CREATE | File::O_READ | File::O_WRITE | File::O_AUTODELETE);

      size_t body_size = 0;
      req_stream->readBody([&tmpfile, &body_size] (const void* data, size_t size) {
        tmpfile.write(data, size);
        body_size += size;
      });

      tmpfile.seekTo(0);

      logDebug(
          "analyticsd",
          "Uploading static table; customer=$0, table=$1, shard=$2, size=$3MB",
          session->getEffectiveNamespace(),
          table_name,
          shard,
          body_size / 1000000.0);

      BinaryCSVInputStream csv(FileInputStream::fromFile(std::move(tmpfile)));
      shredder.addRecordsFromCSV(&csv);
      cstable->commit();
    }

    tsdb_->updatePartitionCSTable(
        session->getEffectiveNamespace(),
        table_name,
        partition_key,
        tmpfile_path + ".cst",
        WallClock::unixMicros());
  } catch (const StandardException& e) {
    logError("analyticsd", e, "error");
    res->setStatus(http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0", e.what()));
    return;
  }

  res->setStatus(http::kStatusCreated);
}

void AnalyticsServlet::getAuthInfo(
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

static String getQueryParam(
    URI::ParamList params,
    const http::HTTPRequest* req) {
  if (req->hasHeader("Content-Type")) {
    auto content_type = req->getHeader("Content-Type");

    if (content_type == "application/json") {
      auto json = json::parseJSON(req->body().toString());
      auto query = json::objectGetString(json, "query");
      if (query.isEmpty()) {
        RAISE(kRuntimeError, "missing query")
      } else {
        return query.get();
      }
    }

    if (content_type == "application/sql") {
      auto query = req->body().toString();
      return query;
    }
  }

  String query;
  if (URI::getParam(params, "query", &query)) {
    return query;
  } else {
    RAISE(kRuntimeError, "missing ?query=... parameter");
  }
}

void AnalyticsServlet::executeSQL(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
  try {
    URI uri(req->uri());
    URI::ParamList params = uri.queryParams();

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
    logError("z1.sql", e, "Uncaught SQL error");
    res->setStatus(http::kStatusBadRequest);
    res->addBody("invalid request");
    res_stream->writeResponse(*res);
  }
}

void AnalyticsServlet::executeSQL_ASCII(
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
//    auto txn = sql_->newTransaction();
//    auto estrat = app_->getExecutionStrategy(session->getEffectiveNamespace());
//    txn->setTableProvider(estrat->tableProvider());
//    auto qplan = sql_->buildQueryPlan(txn.get(), query, estrat);
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

void AnalyticsServlet::executeSQL_BINARY(
    const URI::ParamList& params,
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
  String query;
  try {
    query = getQueryParam(params, req);
  } catch (const Exception& e) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody(e.getMessage());
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
      auto rc = client_auth_->changeNamespace(session, database);
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
      auto txn = sql_service_->startTransaction(session);
      auto qplan = sql_->buildQueryPlan(txn.get(), query);
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

void AnalyticsServlet::executeSQL_JSON(
    const URI::ParamList& params,
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
  String query;
  try {
    query = getQueryParam(params, req);
  } catch (const Exception& e) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody(e.getMessage());
    res_stream->writeResponse(*res);
    return;
  }

  String database;
  if (URI::getParam(params, "database", &database) && !database.empty()) {
    auto rc = client_auth_->changeNamespace(session, database);
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
    auto txn = sql_service_->startTransaction(session);
    auto qplan = sql_->buildQueryPlan(txn.get(), query);

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

void AnalyticsServlet::executeSQL_JSONSSE(
    const URI::ParamList& params,
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
  String query;
  try {
    query = getQueryParam(params, req);
  } catch (const Exception& e) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody(e.getMessage());
    res_stream->writeResponse(*res);
    return;
  }

  auto sse_stream = mkRef(new http::HTTPSSEStream(res, res_stream));
  sse_stream->start();

  String database;
  if (URI::getParam(params, "database", &database) && !database.empty()) {
    auto rc = client_auth_->changeNamespace(session, database);
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
    auto txn = sql_service_->startTransaction(session);
    auto qplan = sql_->buildQueryPlan(txn.get(), query);

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

void AnalyticsServlet::executeQTree(
    Session* session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
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
    //  auto rc = client_auth_->changeNamespace(session, database);
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
      auto txn = sql_service_->startTransaction(session);

      csql::QueryTreeCoder coder(txn.get());
      auto req_body_is = BufferInputStream::fromBuffer(&req->body());
      auto qtree = coder.decode(req_body_is.get());
      auto qplan = sql_->buildQueryPlan(txn.get(), { qtree });

      result_format.sendResults(qplan.get());
    } catch (const StandardException& e) {
      logError("evql", "SQL Error: $0", e.what());
      result_format.sendError(e.what());
    }
  }

  res_stream->finishResponse();
}

} // namespace eventql
