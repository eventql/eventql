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
#include <cstable/CSTableBuilder.h>

using namespace stx;

namespace zbase {

const char AnalyticsServlet::kSessionCookieKey[] = "__dxa_session";
const uint64_t AnalyticsServlet::kSessionLifetimeMicros = 365 * kMicrosPerDay;

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
    documents_api_(docdb) {}

void AnalyticsServlet::handleHTTPRequest(
    RefPtr<http::HTTPRequestStream> req_stream,
    RefPtr<http::HTTPResponseStream> res_stream) {
  const auto& req = req_stream->request();
  URI uri(req.uri());

  http::HTTPResponse res;
  res.populateFromRequest(req);
  //res.addHeader("Access-Control-Allow-Origin", "*"); // FIXME
  //res.addHeader("Access-Control-Allow-Methods", "GET, POST");

  logDebug("analyticsd", "HTTP Request: $0 $1", req.method(), req.uri());

  if (req.method() == http::HTTPMessage::M_OPTIONS) {
    req_stream->readBody();
    res.setStatus(http::kStatusOK);
    res_stream->writeResponse(res);
    return;
  }


  /* AUTH METHODS */
  if (uri.path() == "/analytics/api/v1/auth/login") {
    expectHTTPPost(req);
    req_stream->readBody();
    performLogin(uri, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  auto session_opt = authenticateRequest(req_stream->request());
  if (session_opt.isEmpty()) {
    req_stream->readBody();
    res.addHeader("WWW-Authenticate", "Token");
    res.setStatus(http::kStatusUnauthorized);
    res.addBody("authorization required");
    res_stream->writeResponse(res);
    return;
  }

  const auto& session = session_opt.get();

  if (StringUtil::beginsWith(uri.path(), "/analytics/api/v1/logfiles")) {
    logfile_api_.handle(session, req_stream, res_stream);
    return;
  }

  if (StringUtil::beginsWith(uri.path(), "/analytics/api/v1/documents")) {
    req_stream->readBody();
    documents_api_.handle(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/analytics/api/v1/auth/info") {
    req_stream->readBody();
    getAuthInfo(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/analytics/api/v1/auth/private_api_token") {
    req_stream->readBody();
    getPrivateAPIToken(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }



  if (uri.path() == "/analytics/api/v1/query") {
    req_stream->readBody();
    executeQuery(session, uri, req_stream.get(), res_stream.get());
    return;
  }

  if (uri.path() == "/analytics/api/v1/feeds/fetch") {
    req_stream->readBody();
    fetchFeed(uri, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/analytics/api/v1/reports/generate") {
    req_stream->readBody();
    generateReport(uri, req_stream.get(), res_stream.get());
    return;
  }

  if (uri.path() == "/analytics/api/v1/reports/download") {
    req_stream->readBody();
    downloadReport(uri, req_stream.get(), res_stream.get());
    return;
  }

  if (uri.path() == "/analytics/api/v1/push_events") {
    req_stream->readBody();
    pushEvents(uri, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/analytics/api/v1/pipeline_info") {
    req_stream->readBody();
    pipelineInfo(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }


  /* SESSION TRACKING */
  if (uri.path() == "/analytics/api/v1/session_tracking/events") {
    req_stream->readBody();
    sessionTrackingListEvents(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/analytics/api/v1/session_tracking/event_info") {
    req_stream->readBody();
    sessionTrackingEventInfo(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/analytics/api/v1/session_tracking/attributes") {
    req_stream->readBody();
    sessionTrackingListAttributes(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }


  /* METRICS */
  if (uri.path() == "/analytics/api/v1/metrics") {
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
  if (uri.path() == "/analytics/api/v1/tables/create_table") {
    expectHTTPPost(req);
    req_stream->readBody();
    createTable(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/analytics/api/v1/tables/upload_table") {
    expectHTTPPost(req);
    uploadTable(uri, session, req_stream.get(), &res);
    res_stream->writeResponse(res);
    return;
  }


  /* SQL */
  if (uri.path() == "/analytics/api/v1/sql") {
    req_stream->readBody();
    executeSQL(session, &req, &res);
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/analytics/api/v1/sql_stream") {
    req_stream->readBody();
    executeSQLStream(uri, session, &req, &res, res_stream);
    return;
  }



  res.setStatus(http::kStatusNotFound);
  res.addBody("not found");
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

  String customer;
  if (!URI::getParam(params, "customer", &customer)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?customer=... parameter");
    return;
  }

  String sequence;
  if (!URI::getParam(params, "sequence", &sequence)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?sequence=... parameter");
    return;
  }

  auto task = app_->buildFeedQuery(customer, feed, std::stoull(sequence));
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


void AnalyticsServlet::pushEvents(
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();

  String customer;
  if (!URI::getParam(params, "customer", &customer)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?customer=... parameter");
    return;
  }

  String event_type;
  if (!URI::getParam(params, "type", &event_type)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: missing ?type=... parameter");
    return;
  }

  if (req->body().size() == 0) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("error: empty record (body_size == 0)");
  }

  RAISE(kNotImplementedError);
  //ingress_->insertEvents(customer, event_type, req->body());
  res->setStatus(http::kStatusCreated);
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

void AnalyticsServlet::createTable(
    const AnalyticsSession& session,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto jreq = json::parseJSON(req->body());

  auto table_name = json::objectGetString(jreq, "table_name");
  if (table_name.isEmpty()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing field: table_name");
  }

  auto jschema = json::objectLookup(jreq, "schema");
  if (jschema == jreq.end()) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing field: schema");
  }

  auto num_shards = json::objectGetUInt64(jreq, "num_shards");

  try {
    msg::MessageSchema schema(nullptr);
    schema.fromJSON(jschema, jreq.end());

    TableDefinition td;
    td.set_customer(session.customer());
    td.set_table_name(table_name.get());

    auto tblcfg = td.mutable_config();
    tblcfg->set_schema(schema.encode().toString());
    tblcfg->set_partitioner(zbase::TBL_PARTITION_FIXED);
    tblcfg->set_storage(zbase::TBL_STORAGE_STATIC);
    tblcfg->set_num_shards(num_shards.isEmpty() ? 1 : num_shards.get());

    auto update_param = json::objectGetBool(jreq, "update");
    auto force = !update_param.isEmpty() && update_param.get();

    app_->updateTable(td, force);
  } catch (const StandardException& e) {
    stx::logError("analyticsd", e, "error");
    res->setStatus(http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0", e.what()));
    return;
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
    stx::logError("sql", e, "SQL execution failed");

    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("error");
    json.addString(e.what());
    json.endObject();

    sse_stream.sendEvent(buf, Some(String("error")));
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

Option<AnalyticsSession> AnalyticsServlet::authenticateRequest(
    const http::HTTPRequest& request) const {
  try {
    String cookie;

    if (request.hasHeader("Authorization")) {
      static const String hdrprefix = "Token ";
      auto hdrval = request.getHeader("Authorization");
      if (StringUtil::beginsWith(hdrval, hdrprefix)) {
        cookie = URI::urlDecode(hdrval.substr(hdrprefix.size()));
      }
    }

    if (cookie.empty()) {
      const auto& cookies = request.cookies();
      http::Cookies::getCookie(cookies, kSessionCookieKey, &cookie);
    }

    if (cookie.empty()) {
      return None<AnalyticsSession>();
    }

    return auth_->decodeAuthToken(cookie);
  } catch (const StandardException& e) {
    logDebug("analyticsd", e, "authentication failed because of error");
    return None<AnalyticsSession>();
  }
}

void AnalyticsServlet::performLogin(
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI::ParamList params;
  URI::parseQueryString(req->body().toString(), &params);

  String userid;
  if (!URI::getParam(params, "userid", &userid)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?userid=... parameter");
    return;
  }

  String password;
  if (!URI::getParam(params, "password", &password)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?password=... parameter");
    return;
  }

  auto session = auth_->performLogin(userid, password);

  if (session.isEmpty()) {
    res->addHeader("WWW-Authenticate", "Token");
    res->setStatus(http::kStatusUnauthorized);
  } else {
    auto token = auth_->encodeAuthToken(session.get());

    res->addCookie(
        kSessionCookieKey,
        token,
        WallClock::unixMicros() + kSessionLifetimeMicros,
        "/",
        ".zbase.io",
        false, // FIXPAUL https only...
        true);

    res->addHeader("X-AuthToken", token);
    res->setStatus(http::kStatusOK);
    res->addBody(token);
  }
}


} // namespace zbase
