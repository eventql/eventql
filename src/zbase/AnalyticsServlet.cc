/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <thread>

#include "AnalyticsServlet.h"
#include "zbase/CTRCounter.h"
#include "stx/Language.h"
#include "stx/wallclock.h"
#include "stx/io/fileutil.h"
#include "stx/util/Base64.h"
#include "stx/logging.h"
#include "stx/assets.h"
#include "stx/http/cookies.h"
#include "stx/protobuf/DynamicMessage.h"
#include "stx/protobuf/MessageEncoder.h"
#include "stx/csv/CSVInputStream.h"
#include "stx/csv/BinaryCSVInputStream.h"
#include "zbase/TrafficSegment.h"
#include "zbase/AnalyticsQuery.h"
#include "zbase/AnalyticsQueryResult.h"
#include "zbase/AnalyticsQueryReducer.h"
#include "zbase/Report.h"
#include "zbase/AnalyticsQueryParams.pb.h"
#include "zbase/PipelineInfo.h"
#include "zbase/TableDefinition.h"
#include "csql/runtime/ASCIITableFormat.h"
#include "csql/runtime/JSONSSEStreamFormat.h"
#include "zbase/core/TimeWindowPartitioner.h"
#include "zbase/core/FixedShardPartitioner.h"
#include "zbase/HTTPAuth.h"
#include <cstable/CSTableBuilder.h>

using namespace stx;

namespace zbase {


AnalyticsServlet::AnalyticsServlet(
    RefPtr<AnalyticsApp> app,
    dproc::DispatchService* dproc,
    EventIngress* ingress,
    const String& cachedir,
    AnalyticsAuth* auth,
    csql::Runtime* sql,
    zbase::TSDBService* tsdb,
    ConfigDirectory* customer_dir,
    DocumentDB* docdb) :
    app_(app),
    dproc_(dproc),
    ingress_(ingress),
    cachedir_(cachedir),
    auth_(auth),
    sql_(sql),
    tsdb_(tsdb),
    customer_dir_(customer_dir),
    logfile_api_(app->logfileService(), customer_dir, cachedir),
    events_api_(app->eventsService(), customer_dir, cachedir),
    mapreduce_api_(app->mapreduceService(), customer_dir, cachedir),
    documents_api_(docdb) {}

void AnalyticsServlet::handleHTTPRequest(
    RefPtr<http::HTTPRequestStream> req_stream,
    RefPtr<http::HTTPResponseStream> res_stream) {
  try {
    handle(req_stream, res_stream);
  } catch (const StandardException& e) {
    logError("zbase", e, "error while handling HTTP request");
    req_stream->discardBody();

    http::HTTPResponse res;
    res.populateFromRequest(req_stream->request());
    res.setStatus(http::kStatusInternalServerError);
    res.addHeader("Content-Type", "text/html; charset=utf-8");
    res.addBody(Assets::getAsset("zbase/webui/500.html"));
    res_stream->writeResponse(res);
  }
}

void AnalyticsServlet::handle(
    RefPtr<http::HTTPRequestStream> req_stream,
    RefPtr<http::HTTPResponseStream> res_stream) {
  const auto& req = req_stream->request();
  URI uri(req.uri());

  logDebug("zbase", "HTTP Request: $0 $1", req.method(), req.uri());

  http::HTTPResponse res;
  res.populateFromRequest(req);

  if (req.method() == http::HTTPMessage::M_OPTIONS) {
    req_stream->readBody();
    res.setStatus(http::kStatusOK);
    res_stream->writeResponse(res);
    return;
  }

  /* AUTH METHODS */
  if (uri.path() == "/api/v1/auth/login") {
    expectHTTPPost(req);
    req_stream->readBody();
    performLogin(uri, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/auth/logout") {
    expectHTTPPost(req);
    req_stream->readBody();
    performLogout(uri, &req, &res);
    res_stream->writeResponse(res);
    return;
  }


  auto session_opt = HTTPAuth::authenticateRequest(
      req_stream->request(),
      auth_);

  if (session_opt.isEmpty()) {
    req_stream->readBody();
    res.setStatus(http::kStatusUnauthorized);
    res.addHeader("WWW-Authenticate", "Token");
    res.addHeader("Content-Type", "text/html; charset=utf-8");
    res.addBody(Assets::getAsset("zbase/webui/401.html"));
    res_stream->writeResponse(res);
    return;
  }

  const auto& session = session_opt.get();

  if (StringUtil::beginsWith(uri.path(), "/api/v1/logfiles")) {
    logfile_api_.handle(session, req_stream, res_stream);
    return;
  }

  if (StringUtil::beginsWith(uri.path(), "/api/v1/events")) {
    events_api_.handle(session, req_stream, res_stream);
    return;
  }

  if (StringUtil::beginsWith(uri.path(), "/api/v1/mapreduce")) {
    mapreduce_api_.handle(session, req_stream, res_stream);
    return;
  }

  if (StringUtil::beginsWith(uri.path(), "/api/v1/documents")) {
    req_stream->readBody();
    documents_api_.handle(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/auth/info") {
    req_stream->readBody();
    getAuthInfo(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/auth/private_api_token") {
    req_stream->readBody();
    getPrivateAPIToken(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }


  if (uri.path() == "/api/v1/query") {
    req_stream->readBody();
    executeQuery(session, uri, req_stream.get(), res_stream.get());
    return;
  }

  if (uri.path() == "/api/v1/feeds/fetch") {
    req_stream->readBody();
    fetchFeed(session, uri, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/reports/generate") {
    req_stream->readBody();
    generateReport(uri, req_stream.get(), res_stream.get());
    return;
  }

  if (uri.path() == "/api/v1/reports/download") {
    req_stream->readBody();
    downloadReport(uri, req_stream.get(), res_stream.get());
    return;
  }

  if (uri.path() == "/api/v1/pipeline_info") {
    req_stream->readBody();
    pipelineInfo(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }


  /* SESSION TRACKING */
  if (uri.path() == "/api/v1/session_tracking/events") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      sessionTrackingListEvents(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/session_tracking/events/add_event") {
    expectHTTPPost(req);
    req_stream->readBody();
    sessionTrackingEventAdd(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/session_tracking/events/remove_event") {
    expectHTTPPost(req);
    req_stream->readBody();
    sessionTrackingEventRemove(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/session_tracking/events/add_field") {
    expectHTTPPost(req);
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      sessionTrackingEventAddField(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/session_tracking/events/remove_field") {
    expectHTTPPost(req);
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      sessionTrackingEventRemoveField(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/session_tracking/event_info") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      sessionTrackingEventInfo(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/session_tracking/attributes") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      sessionTrackingListAttributes(session, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }


  /* METRICS */
  if (uri.path() == "/api/v1/metrics") {
    req_stream->readBody();
    switch (req.method()) {
      case http::HTTPMessage::M_POST:
        insertIntoMetric(uri, session, &req, &res);
        res_stream->writeResponse(res);
        return;
      default:
        break;
    }
  }


  /* TABLES */
  if (uri.path() == "/api/v1/tables") {
    req_stream->readBody();
    listTables(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/tables/insert") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &req, &res] {
      insertIntoTable(session, &req, &res);
    });
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

  if (uri.path() == "/api/v1/tables/upload_table") {
    expectHTTPPost(req);
    uploadTable(uri, session, req_stream.get(), &res);
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


  /* SQL */
  if (uri.path() == "/api/v1/sql") {
    req_stream->readBody();
    executeSQL(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/sql_stream") {
    req_stream->readBody();
    executeSQLStream(uri, session, &req, &res, res_stream);
    return;
  }

  if (uri.path() == "/api/v1/sql/aggregate_partition") {
    req_stream->readBody();
    executeSQLAggregatePartition(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/sql/scan_partition") {
    req_stream->readBody();
    executeSQLScanPartition(session, &req, &res, res_stream);
    return;
  }


  res.setStatus(http::kStatusNotFound);
  res.addHeader("Content-Type", "text/html; charset=utf-8");
  res.addBody(Assets::getAsset("zbase/webui/404.html"));
  res_stream->writeResponse(res);
}

void AnalyticsServlet::executeQuery(
    const AnalyticsSession& session,
    const URI& uri,
    http::HTTPRequestStream* req_stream,
    http::HTTPResponseStream* res_stream) {
  const auto& params = uri.queryParams();

  dproc::TaskSpec task;

  bool is_query = false;

  String report_name;
  if (URI::getParam(params, "report", &report_name)) {
    auto until = WallClock::unixMicros() - 1 * kMicrosPerDay;
    String until_str;
    if (URI::getParam(params, "until", &until_str)) {
      until = std::stoul(until_str) * kMicrosPerSecond;
    }

    auto from = until - 30 * kMicrosPerDay;
    String from_str;
    if (URI::getParam(params, "from", &from_str)) {
      from = std::stoul(from_str) * kMicrosPerSecond;
    }

    task = app_->buildReportQuery(
        session.customer(),
        report_name,
        from,
        until,
        params);
  } else {
    is_query = true;
    task = app_->buildAnalyticsQuery(session, params);
  }

  auto task_future = dproc_->run(task);

  http::HTTPSSEStream sse_stream(req_stream, res_stream);
  sse_stream.start();

  auto send_status_update = [&sse_stream, &task_future] {
    auto status = task_future->status();
    auto progress = status.progress();

    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("status");
    json.addString("running");
    json.addComma();
    json.addObjectEntry("progress");
    json.addFloat(progress);
    json.addComma();
    json.addObjectEntry("message");
    if (progress == 0.0f) {
      json.addString("Waiting...");
    } else if (progress == 1.0f) {
      json.addString("Downloading...");
    } else {
      json.addString("Running...");
    }
    json.endObject();

    sse_stream.sendEvent(buf, Some(String("status")));
  };

  auto future = task_future->result();
  do {
    if (res_stream->isClosed()) {
      stx::logDebug("analyticsd", "Aborting Query...");
      task_future->cancel();
      return;
    }

    send_status_update();
  } while (!future.waitFor(1 * kMicrosPerSecond));

  if (is_query) {
    auto task_result = future.get()->getInstanceAs<AnalyticsQueryReducer>();
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    task_result->toJSON(&json);

    sse_stream.sendEvent(buf, Some(String("result")));
  } else {
    auto report = future.get();
    auto result_data = report->getData();

    sse_stream.sendEvent(
        result_data->data(),
        result_data->size(),
        Some(String("result")));
  }

  sse_stream.finish();
}

void AnalyticsServlet::fetchFeed(
    const AnalyticsSession& session,
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();

  String feed;
  if (!URI::getParam(params, "feed", &feed)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?feed=... parameter");
    return;
  }

  String sequence;
  if (!URI::getParam(params, "sequence", &sequence)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?sequence=... parameter");
    return;
  }

  auto task = app_->buildFeedQuery(
      session.customer(),
      feed,
      std::stoull(sequence));

  auto task_future = dproc_->run(task);
  auto result = task_future->result().waitAndGet();
  auto result_data = result->getData(); // FIXPAUL

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", result->contentType());
  res->addBody(result_data->data(), result_data->size());
}

void AnalyticsServlet::generateReport(
    const URI& uri,
    http::HTTPRequestStream* req_stream,
    http::HTTPResponseStream* res_stream) {
  const auto& params = uri.queryParams();

  String report_name;
  if (!URI::getParam(params, "report", &report_name)) {
    http::HTTPResponse res;
    res.populateFromRequest(req_stream->request());
    res.setStatus(http::kStatusBadRequest);
    res.addBody("missing/invalid ?report=... parameter");
    res_stream->writeResponse(res);
    return;
  }

  String customer;
  if (!URI::getParam(params, "customer", &customer)) {
    http::HTTPResponse res;
    res.populateFromRequest(req_stream->request());
    res.setStatus(http::kStatusBadRequest);
    res.addBody("missing ?customer=... parameter");
    res_stream->writeResponse(res);
    return;
  }

  auto until = WallClock::unixMicros() - 1 * kMicrosPerDay;
  String until_str;
  if (URI::getParam(params, "until", &until_str)) {
    until = std::stoul(until_str) * kMicrosPerSecond;
  }

  auto from = until - 30 * kMicrosPerDay;
  String from_str;
  if (URI::getParam(params, "from", &from_str)) {
    from = std::stoul(from_str) * kMicrosPerSecond;
  }

  auto task = app_->buildReportQuery(
      customer,
      report_name,
      from,
      until,
      params);

  auto task_future = dproc_->run(task);

  http::HTTPSSEStream sse_stream(req_stream, res_stream);
  sse_stream.start();

  auto send_progress = [&sse_stream] (double progress) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("status");
    json.addString("running");
    json.addComma();
    json.addObjectEntry("progress");
    json.addFloat(progress);
    json.addComma();
    json.addObjectEntry("message");
    if (progress == 0.0f) {
      json.addString("Waiting...");
    } else {
      json.addString("Running...");
    }
    json.endObject();

    sse_stream.sendEvent(buf, Some(String("status")));
  };

  auto future = task_future->result();
  do {
    if (res_stream->isClosed()) {
      stx::logDebug("analyticsd", "Aborting Query...");
      task_future->cancel();
      return;
    }

    send_progress(task_future->status().progress());
  } while (!future.waitFor(1 * kMicrosPerSecond));

  auto task_result = future.get();
  send_progress(1.0f);

  auto ext = "dat";
  if (StringUtil::beginsWith(task_result->contentType(), "application/json")) {
    ext = "json";
  }

  if (StringUtil::beginsWith(task_result->contentType(), "application/csv")) {
    ext = "csv";
  }

  auto filename = StringUtil::format(
      "report-$0-$1.$2.$3",
      WallClock::now().toString("%y%m%d%H%M%S"),
      StringUtil::stripShell(report_name),
      Random::singleton()->hex64(),
      ext);

  task_result->saveToFile(FileUtil::joinPaths(cachedir_, filename));

  stx::logDebug("analytics", "Generated report: $0", filename);

  auto redir_url = StringUtil::format(
      "http://backend.analytics.fnrd.net/reports/download?f=$0",
      URI::urlEncode(filename));

  sse_stream.sendEvent(redir_url, Some(String("redirect")));
  sse_stream.finish();
}

void AnalyticsServlet::downloadReport(
    const URI& uri,
    http::HTTPRequestStream* req_stream,
    http::HTTPResponseStream* res_stream) {
  const auto& params = uri.queryParams();

  String filename;
  if (!URI::getParam(params, "f", &filename)) {
    http::HTTPResponse res;
    res.populateFromRequest(req_stream->request());
    res.setStatus(http::kStatusBadRequest);
    res.addBody("missing ?f=... parameter");
    res_stream->writeResponse(res);
    return;
  }

  auto filepath = FileUtil::joinPaths(
      cachedir_,
      StringUtil::stripShell(filename));

  if (!FileUtil::exists(filepath)) {
    http::HTTPResponse res;
    res.populateFromRequest(req_stream->request());
    res.setStatus(http::kStatusNotFound);
    res.addBody("report not found");
    res_stream->writeResponse(res);
    return;
  }

  auto filesize = FileUtil::size(filepath);

  String content_type = "application/octet-stream";
  if (StringUtil::endsWith(filename, ".json")) {
    content_type = "application/json; charset=utf-8";
  }

  if (StringUtil::endsWith(filename, ".csv")) {
    content_type = "application/csv; charset=utf-8";
  }

  http::HTTPResponse res;
  res.populateFromRequest(req_stream->request());
  res.setStatus(http::kStatusOK);
  res.addHeader("Content-Type", content_type);
  res.addHeader("Content-Length", StringUtil::toString(filesize));
  res.addHeader(
      "Content-Disposition",
      StringUtil::format("attachment; filename=\"$0\"", filename));

  res_stream->startResponse(res);
  // FIXPAUL use sendfile
  res_stream->writeBodyChunk(FileUtil::read(filepath));
  res_stream->finishResponse();
}

void AnalyticsServlet::insertIntoMetric(
    const URI& uri,
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto params = uri.queryParams();

  String metric;
  if (!URI::getParam(params, "metric", &metric)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?metric=... parameter");
    return;
  }

  String value;
  if (!URI::getParam(params, "value", &value)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?value=... parameter");
    return;
  }

  auto time = WallClock::unixMicros();

  stx::logTrace("analyticsd", "Insert into metric '$0' -> $1", metric, value);
  app_->insertMetric(session.customer(), metric, time, value);
  res->setStatus(http::kStatusCreated);
}

void AnalyticsServlet::listTables(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));

  json.beginObject();
  json.addObjectEntry("tables");
  json.beginArray();

  auto table_provider = app_->getTableProvider(session.customer());
  size_t ntable = 0;
  table_provider->listTables([&json, &ntable] (const csql::TableInfo table) {
    if (++ntable > 1) {
      json.addComma();
    }

    json.beginObject();

    json.addObjectEntry("name");
    json.addString(table.table_name);

    json.endObject();
  });

  json.endArray();
  json.endObject();

  res->setStatus(http::kStatusOK);
  res->setHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(buf);
}

void AnalyticsServlet::fetchTableDefinition(
    const AnalyticsSession& session,
    const String& table_name,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {

  auto table_provider = app_->getTableProvider(session.customer());
  auto table_opt = table_provider->describe(table_name);
  if (table_opt.isEmpty()) {
    res->setStatus(http::kStatusNotFound);
    res->addBody("table not found");
    return;
  }

  const auto& table = table_opt.get();

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));

  json.beginObject();
  json.addObjectEntry("table");
  json.beginObject();

  json.addObjectEntry("name");
  json.addString(table.table_name);
  json.addComma();

  json.addObjectEntry("columns");
  json.beginArray();
  for (size_t i = 0; i < table.columns.size(); ++i) {
    const auto& col = table.columns[i];

    if (i > 0) {
      json.addComma();
    }

    json.beginObject();

    json.addObjectEntry("column_name");
    json.addString(col.column_name);
    json.addComma();

    json.addObjectEntry("type");
    json.addString(col.type);
    json.addComma();

    json.addObjectEntry("is_nullable");
    json.addBool(col.is_nullable);

    json.endObject();
  }
  json.endArray();

  json.endObject();
  json.endObject();

  res->setStatus(http::kStatusOK);
  res->setHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(buf);
}

void AnalyticsServlet::createTable(
    const AnalyticsSession& session,
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

    TableDefinition td;
    td.set_customer(session.customer());
    td.set_table_name(table_name.get());

    auto tblcfg = td.mutable_config();
    tblcfg->set_schema(schema.encode().toString());
    tblcfg->set_num_shards(num_shards.isEmpty() ? 1 : num_shards.get());

    if (table_type == "timeseries" || table_type == "log_timeseries") {
      tblcfg->set_partitioner(zbase::TBL_PARTITION_TIMEWINDOW);
      tblcfg->set_storage(zbase::TBL_STORAGE_LOG);

      auto partition_size = json::objectGetUInt64(jreq, "partition_size");
      auto partcfg = tblcfg->mutable_time_window_partitioner_config();
      partcfg->set_partition_size(
          partition_size.isEmpty() ? 4 * kMicrosPerHour : partition_size.get());
    }

    else if (table_type == "static" || table_type == "static_fixed") {
      tblcfg->set_partitioner(zbase::TBL_PARTITION_FIXED);
      tblcfg->set_storage(zbase::TBL_STORAGE_STATIC);
    }

    else if (table_type == "static_timeseries") {
      tblcfg->set_partitioner(zbase::TBL_PARTITION_TIMEWINDOW);
      tblcfg->set_storage(zbase::TBL_STORAGE_STATIC);

      auto partition_size = json::objectGetUInt64(jreq, "partition_size");
      auto partcfg = tblcfg->mutable_time_window_partitioner_config();
      partcfg->set_partition_size(
          partition_size.isEmpty() ? 4 * kMicrosPerHour : partition_size.get());
    }

    else {
      RAISEF(kIllegalArgumentError, "invalid table type: $0", table_type);
    }

    app_->updateTable(td, force);
  } catch (const StandardException& e) {
    stx::logError("analyticsd", e, "error");
    res->setStatus(http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0", e.what()));
    return;
  }

  res->setStatus(http::kStatusCreated);
}

void AnalyticsServlet::insertIntoTable(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto jreq = json::parseJSON(req->body());

  auto ncols = json::arrayLength(jreq.begin(), jreq.end());
  for (size_t i = 0; i < ncols; ++i) {
    auto jrow = json::arrayLookup(jreq.begin(), jreq.end(), i); // O(N^2) but who cares...

    auto data = json::objectLookup(jrow, jreq.end(), "data");
    if (data == jreq.end()) {
      RAISE(kRuntimeError, "missing field: data");
    }

    auto table = json::objectGetString(jrow, jreq.end(), "table");
    if (table.isEmpty()) {
      RAISE(kRuntimeError, "missing field: table");
    }

    tsdb_->insertRecord(
        session.customer(),
        table.get(),
        data,
        data + data->size);
  }

  res->setStatus(http::kStatusCreated);
}

void AnalyticsServlet::uploadTable(
    const URI& uri,
    const AnalyticsSession& session,
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
  auto schema = tsdb_->tableSchema(session.customer(), table_name);
  if (schema.isEmpty()) {
    res->setStatus(http::kStatusNotFound);
    res->addBody("error: table not found");
    return;
  }

  auto tmpfile_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("upload_$0.tmp", Random::singleton()->hex128()));

  auto partition_key = zbase::FixedShardPartitioner::partitionKeyFor(
      table_name,
      shard);

  try {
    cstable::CSTableBuilder cstable(schema.get().get());

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

      stx::logDebug(
          "analyticsd",
          "Uploading static table; customer=$0, table=$1, shard=$2, size=$3MB",
          session.customer(),
          table_name,
          shard,
          body_size / 1000000.0);

      BinaryCSVInputStream csv(FileInputStream::fromFile(std::move(tmpfile)));
      cstable.addRecordsFromCSV(&csv);
      cstable.write(tmpfile_path + ".cst");
    }

    tsdb_->updatePartitionCSTable(
        session.customer(),
        table_name,
        partition_key,
        tmpfile_path + ".cst",
        WallClock::unixMicros());
  } catch (const StandardException& e) {
    stx::logError("analyticsd", e, "error");
    res->setStatus(http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0", e.what()));
    return;
  }

  res->setStatus(http::kStatusCreated);
}

void AnalyticsServlet::getAuthInfo(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  Buffer buf;

  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  json.beginObject();
  json.addObjectEntry("valid");
  json.addTrue();
  json.addComma();
  json.addObjectEntry("customer");
  json.addString(session.customer());
  json.addComma();
  json.addObjectEntry("userid");
  json.addString(session.userid());
  json.endObject();

  res->addBody(buf);
  res->setStatus(http::kStatusOK);
}

void AnalyticsServlet::getPrivateAPIToken(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  const auto& params = uri.queryParams();
  String devnull;

  AnalyticsPrivileges privs;

  if (URI::getParam(params, "allow_read", &devnull)) {
    privs.set_allow_private_api_read_access(true);
  }

  if (URI::getParam(params, "allow_write", &devnull)) {
    privs.set_allow_private_api_write_access(true);
  }

  auto token = auth_->getPrivateAPIToken(session.customer(), privs);

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  json.beginObject();
  json.addObjectEntry("api_token");
  json.addString(URI::urlEncode(token));
  json.endObject();

  res->addBody(buf);
  res->setStatus(http::kStatusOK);
}


void AnalyticsServlet::executeSQL(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto query = req->body().toString();

  Buffer result;
  sql_->executeQuery(
      query,
      app_->getExecutionStrategy(session.customer()),
      new csql::ASCIITableFormat(BufferOutputStream::fromBuffer(&result)));

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "text/plain");
  res->addHeader("Connection", "close");
  res->addBody(result);
}

void AnalyticsServlet::executeSQLAggregatePartition(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto query = req->body().toString();

  Buffer result;
  auto os = BufferOutputStream::fromBuffer(&result);
  sql_->executeAggregate(
      msg::decode<csql::RemoteAggregateParams>(query),
      app_->getExecutionStrategy(session.customer()),
      os.get());

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/octet-stream");
  res->addBody(result);
}

void AnalyticsServlet::executeSQLScanPartition(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
  // FIXME error handling
  auto query = msg::decode<RemoteTSDBScanParams>(req->body().toString());

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/octet-stream");
  res->addHeader("Connection", "close");
  res_stream->startResponse(*res);

  executeSQLScanPartition(
      session,
      query,
      [&res_stream] (int argc, const csql::SValue* argv) -> bool {
    Buffer buf(sizeof(uint64_t));
    auto os = BufferOutputStream::fromBuffer(&buf);
    for (int i = 0; i < argc; ++i) argv[i].encode(os.get());
    *((uint64_t*) buf.data()) = buf.size() - sizeof(uint64_t);

    res_stream->writeBodyChunk(buf);
    res_stream->waitForReader();
    return true;
  });

  util::BinaryMessageWriter buf;
  buf.appendUInt64(0);
  res_stream->writeBodyChunk(Buffer(buf.data(), buf.size()));
  res_stream->finishResponse();
}

void AnalyticsServlet::executeSQLScanPartition(
    const AnalyticsSession& session,
    const RemoteTSDBScanParams& query,
    Function<bool (int argc, const csql::SValue* argv)> fn) {
  Vector<RefPtr<csql::SelectListNode>> select_list;
  for (const auto& e : query.select_list()) {
    csql::Parser parser;
    parser.parseValueExpression(
        e.expression().data(),
        e.expression().size());

    auto stmts = parser.getStatements();
    if (stmts.size() != 1) {
      RAISE(kIllegalArgumentError);
    }

    auto slnode = mkRef(
        new csql::SelectListNode(
            sql_->queryPlanBuilder()->buildValueExpression(stmts[0])));

    if (e.has_alias()) {
      slnode->setAlias(e.alias());
    }

    select_list.emplace_back(slnode);
  }

  Option<RefPtr<csql::ValueExpressionNode>> where_expr;
  if (query.has_where_expression()) {
    csql::Parser parser;
    parser.parseValueExpression(
        query.where_expression().data(),
        query.where_expression().size());

    auto stmts = parser.getStatements();
    if (stmts.size() != 1) {
      RAISE(kIllegalArgumentError);
    }

    where_expr = Some(mkRef(
        sql_->queryPlanBuilder()->buildValueExpression(stmts[0])));
  }

  auto qtree = mkRef(
      new csql::SequentialScanNode(
            query.table_name(),
            select_list,
            where_expr,
            (csql::AggregationStrategy) query.aggregation_strategy()));

  auto execution_strategy = app_->getExecutionStrategy(session.customer());
  auto expr = sql_->queryBuilder()->buildTableExpression(
      qtree.get(),
      execution_strategy->tableProvider(),
      sql_);

  sql_->executeStatement(expr.get(), fn);
}

void AnalyticsServlet::executeSQLStream(
    const URI& uri,
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res,
    RefPtr<http::HTTPResponseStream> res_stream) {
  http::HTTPSSEStream sse_stream(res, res_stream);
  sse_stream.start();

  try {
    const auto& params = uri.queryParams();

    String query;
    if (!URI::getParam(params, "query", &query)) {
      RAISE(kRuntimeError, "missing ?query=... parameter");
    }

    sql_->executeQuery(
        query,
        app_->getExecutionStrategy(session.customer()),
        new csql::JSONSSEStreamFormat(&sse_stream));

  } catch (const StandardException& e) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("error");
    json.addString(e.what());
    json.endObject();

    sse_stream.sendEvent(buf, Some(String("query_error")));
  }

  sse_stream.finish();
}

void AnalyticsServlet::pipelineInfo(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {

  auto customer_conf = customer_dir_->configFor(session.customer());
  const auto& pipelines = PipelineInfo::forCustomer(customer_conf->config);

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  json.beginObject();
  json.addObjectEntry("pipelines");
  json.beginArray();

  for (size_t i = 0; i < pipelines.size(); ++i) {
    const auto& pipeline = pipelines[i];

    if (i > 0) {
      json.addComma();
    }

    json.beginObject();

    json.addObjectEntry("type");
    json.addString(pipeline.type);
    json.addComma();

    json.addObjectEntry("path");
    json.addString(pipeline.path);
    json.addComma();

    json.addObjectEntry("name");
    json.addString(pipeline.name);
    json.addComma();

    json.addObjectEntry("info");
    json.addString(pipeline.info);
    json.addComma();

    json.addObjectEntry("status");
    json.addString(pipeline.status);

    json.endObject();
  }

  json.endArray();
  json.endObject();

  res->setStatus(http::kStatusOK);
  res->setHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(buf);
}

void AnalyticsServlet::sessionTrackingListEvents(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto customer_conf = customer_dir_->configFor(session.customer());
  const auto& logjoin_conf = customer_conf->config.logjoin_config();

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  json.beginObject();
  json.addObjectEntry("session_events");
  json.beginArray();

  size_t i = 0;
  for (const auto& ev_def : logjoin_conf.session_event_schemas()) {
    auto schema = msg::MessageSchema::decode(ev_def.schema());

    if (++i > 1) {
      json.addComma();
    }

    json.beginObject();

    json.addObjectEntry("event");
    json.addString(ev_def.evtype());
    json.addComma();

    json.addObjectEntry("schema_debug");
    json.addString(schema->toString());

    json.endObject();
  }

  json.endArray();
  json.endObject();

  res->setStatus(http::kStatusOK);
  res->setHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(buf);
}

void AnalyticsServlet::sessionTrackingListAttributes(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto customer_conf = customer_dir_->configFor(session.customer());
  const auto& logjoin_conf = customer_conf->config.logjoin_config();

  auto schema = msg::MessageSchema::decode(
      logjoin_conf.session_attributes_schema());

  Buffer buf;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
  json.beginObject();
  json.addObjectEntry("session_attributes");
  schema->toJSON(&json);
  json.endObject();

  res->setStatus(http::kStatusOK);
  res->setHeader("Content-Type", "application/json; charset=utf-8");
  res->addBody(buf);
}

void AnalyticsServlet::sessionTrackingEventInfo(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  const auto& params = uri.queryParams();

  String event_name;
  if (!URI::getParam(params, "event", &event_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?event=... parameter");
    return;
  }

  auto customer_conf = customer_dir_->configFor(session.customer());
  const auto& logjoin_conf = customer_conf->config.logjoin_config();

  for (const auto& ev_def : logjoin_conf.session_event_schemas()) {
    if (ev_def.evtype() == event_name) {
      auto schema = msg::MessageSchema::decode(ev_def.schema());

      Buffer buf;
      json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
      json.beginObject();
      json.addObjectEntry("event");
      json.beginObject();

      json.addObjectEntry("name");
      json.addString(ev_def.evtype());
      json.addComma();

      json.addObjectEntry("schema");
      schema->toJSON(&json);
      json.addComma();

      json.addObjectEntry("schema_debug");
      json.addString(schema->toString());

      json.endObject();
      json.endObject();

      res->setStatus(http::kStatusOK);
      res->setHeader("Content-Type", "application/json; charset=utf-8");
      res->addBody(buf);
      return;
    }
  }

  res->setStatus(http::kStatusNotFound);
  res->addBody("not found");
}

void AnalyticsServlet::sessionTrackingEventAdd(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  const auto& params = uri.queryParams();

  String event_name;
  if (!URI::getParam(params, "event", &event_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?event=... parameter");
    return;
  }

  auto customer_conf = customer_dir_->configFor(session.customer())->config;
  auto logjoin_conf = customer_conf.mutable_logjoin_config();

  for (auto& ev_def : logjoin_conf->session_event_schemas()) {
    if (ev_def.evtype() == event_name) {
      RAISE(kRuntimeError, "an event with this name already exists");
    }
  }

  auto field_id = logjoin_conf->session_schema_next_field_id();
  auto ev_def = logjoin_conf->add_session_event_schemas();
  ev_def->set_evtype(event_name);
  ev_def->set_evid(field_id);

  msg::MessageSchema schema(nullptr);
  schema.setName(event_name);
  ev_def->set_schema(schema.encode().toString());

  logjoin_conf->set_session_schema_next_field_id(field_id + 1);
  customer_dir_->updateCustomerConfig(customer_conf);

  res->setStatus(http::kStatusCreated);
  res->addBody("ok");
  return;
}

void AnalyticsServlet::sessionTrackingEventRemove(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  const auto& params = uri.queryParams();

  String event_name;
  if (!URI::getParam(params, "event", &event_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?event=... parameter");
    return;
  }

  auto customer_conf = customer_dir_->configFor(session.customer())->config;
  auto logjoin_conf = customer_conf.mutable_logjoin_config();

  auto events = logjoin_conf->mutable_session_event_schemas();
  for (auto i = 0; i < events->size(); ++i) {
    if (events->Get(i).evtype() == event_name) {
      events->DeleteSubrange(i, 1);
      customer_dir_->updateCustomerConfig(customer_conf);
      res->setStatus(http::kStatusCreated);
      res->addBody("ok");
      return;
    }
  }

  res->setStatus(http::kStatusNotFound);
  res->addBody("event not found");
}

void AnalyticsServlet::sessionTrackingEventAddField(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  const auto& params = uri.queryParams();

  String event_name;
  if (!URI::getParam(params, "event", &event_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?event=... parameter");
    return;
  }

  String field_name;
  if (!URI::getParam(params, "field", &field_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?field=... parameter");
    return;
  }

  String field_type_str;
  if (!URI::getParam(params, "type", &field_type_str)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?type=... parameter");
    return;
  }

  auto customer_conf = customer_dir_->configFor(session.customer())->config;
  auto logjoin_conf = customer_conf.mutable_logjoin_config();

  auto field_type = msg::fieldTypeFromString(field_type_str);
  bool field_optional = false;
  bool field_repeated = false;
  auto field_id = logjoin_conf->session_schema_next_field_id();

  String repeated_str;
  if (URI::getParam(params, "repeated", &repeated_str) &&
      repeated_str == "true") {
    field_repeated = true;
  }

  String optional_str;
  if (URI::getParam(params, "optional", &optional_str) &&
      optional_str == "true") {
    field_optional = true;
  }

  for (auto& ev_def : *logjoin_conf->mutable_session_event_schemas()) {
    if (ev_def.evtype() != event_name) {
      continue;
    }

    eventDefinitonAddField(
        &ev_def,
        field_name,
        field_id,
        field_type,
        field_repeated,
        field_optional);

    logjoin_conf->set_session_schema_next_field_id(field_id + 1);
    customer_dir_->updateCustomerConfig(customer_conf);
    res->setStatus(http::kStatusCreated);
    res->addBody("ok");
    return;
  }

  res->setStatus(http::kStatusNotFound);
  res->addBody("event not found");
}

void AnalyticsServlet::sessionTrackingEventRemoveField(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  const auto& params = uri.queryParams();

  String event_name;
  if (!URI::getParam(params, "event", &event_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?event=... parameter");
    return;
  }

  String field_name;
  if (!URI::getParam(params, "field", &field_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?field=... parameter");
    return;
  }

  auto customer_conf = customer_dir_->configFor(session.customer())->config;
  auto logjoin_conf = customer_conf.mutable_logjoin_config();

  for (auto& ev_def : *logjoin_conf->mutable_session_event_schemas()) {
    if (ev_def.evtype() != event_name) {
      continue;
    }

    eventDefinitonRemoveField(&ev_def, field_name);

    customer_dir_->updateCustomerConfig(customer_conf);
    res->setStatus(http::kStatusCreated);
    res->addBody("ok");
    return;
  }

  res->setStatus(http::kStatusNotFound);
  res->addBody("event not found");
}

void AnalyticsServlet::performLogin(
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  String next_step;
  auto session = auth_->performLogin(req->body().toString(), &next_step);

  // login failed, invalid credentials or missing data
  if (session.isEmpty()) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));

    json.beginObject();
    json.addObjectEntry("success");
    json.addFalse();

    if (!next_step.empty()) {
      json.addComma();
      json.addObjectEntry("next_step");
      json.addString(next_step);
    }

    json.endObject();

    res->addHeader("WWW-Authenticate", "Token");
    res->setStatus(http::kStatusUnauthorized);
    res->addBody(buf);
  }

  // login successful
  if (!session.isEmpty()) {
    auto token = auth_->encodeAuthToken(session.get());

    res->addCookie(
        HTTPAuth::kSessionCookieKey,
        token,
        WallClock::unixMicros() + HTTPAuth::kSessionLifetimeMicros,
        "/",
        getCookieDomain(*req),
        false,
        true);

    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));

    json.beginObject();
    json.addObjectEntry("success");
    json.addTrue();
    json.addComma();
    json.addObjectEntry("auth_token");
    json.addString(token);
    json.endObject();

    res->addHeader("X-AuthToken", token);
    res->setStatus(http::kStatusOK);
    res->addBody(buf);
  }
}

void AnalyticsServlet::performLogout(
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  res->addCookie(
      HTTPAuth::kSessionCookieKey,
      "",
      0,
      "/",
      getCookieDomain(*req));

  res->setStatus(http::kStatusOK);
  res->addBody("goodbye");
}



} // namespace zbase
