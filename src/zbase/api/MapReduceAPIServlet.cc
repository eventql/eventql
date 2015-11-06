/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "stx/assets.h"
#include <stx/fnv.h>
#include "stx/protobuf/msg.h"
#include "stx/io/BufferedOutputStream.h"
#include "zbase/api/MapReduceAPIServlet.h"
#include "zbase/mapreduce/MapReduceTask.h"
#include "sstable/sstablereader.h"

using namespace stx;

namespace zbase {

MapReduceAPIServlet::MapReduceAPIServlet(
    MapReduceService* service,
    ConfigDirectory* cdir,
    const String& cachedir) :
    service_(service),
    cdir_(cdir),
    cachedir_(cachedir) {}

static const String kResultPathPrefix = "/api/v1/mapreduce/result/";

void MapReduceAPIServlet::handle(
    const AnalyticsSession& session,
    RefPtr<stx::http::HTTPRequestStream> req_stream,
    RefPtr<stx::http::HTTPResponseStream> res_stream) {
  const auto& req = req_stream->request();
  URI uri(req.uri());

  http::HTTPResponse res;
  res.populateFromRequest(req);

  if (uri.path() == "/api/v1/mapreduce/execute") {
    executeMapReduceScript(session, uri, req_stream.get(), res_stream.get());
    return;
  }

  if (StringUtil::beginsWith(uri.path(), kResultPathPrefix)) {
    fetchResult(
        session,
        uri.path().substr(kResultPathPrefix.size()),
        req_stream.get(),
        res_stream.get());
    return;
  }

  if (uri.path() == "/api/v1/mapreduce/tasks/map_partition") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &uri, &req, &res] {
      executeMapPartitionTask(session, uri, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/mapreduce/tasks/reduce") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &uri, &req, &res] {
      executeReduceTask(session, uri, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/mapreduce/tasks/save_to_table") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &uri, &req, &res] {
      executeSaveToTableTask(session, uri, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/api/v1/mapreduce/tasks/save_to_table_partition") {
    req_stream->readBody();
    catchAndReturnErrors(&res, [this, &session, &uri, &req, &res] {
      executeSaveToTablePartitionTask(session, uri, &req, &res);
    });
    res_stream->writeResponse(res);
    return;
  }

  res.setStatus(http::kStatusNotFound);
  res.addHeader("Content-Type", "text/html; charset=utf-8");
  res.addBody(Assets::getAsset("zbase/webui/404.html"));
  res_stream->writeResponse(res);
}

void MapReduceAPIServlet::executeMapPartitionTask(
    const AnalyticsSession& session,
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI::ParamList params;
  URI::parseQueryString(req->body().toString(), &params);

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

  String map_fn;
  if (!URI::getParam(params, "map_function", &map_fn)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?map_function=... parameter");
    return;
  }

  auto shard_id = service_->mapPartition(
      session,
      table_name,
      SHA1Hash::fromHexString(partition_key),
      map_fn);

  if (shard_id.isEmpty()) {
    res->setStatus(http::kStatusNoContent);
  } else {
    res->setStatus(http::kStatusCreated);
    res->addBody(shard_id.get().toString());
  }
}

void MapReduceAPIServlet::executeReduceTask(
    const AnalyticsSession& session,
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI::ParamList params;
  URI::parseQueryString(req->body().toString(), &params);

  Vector<String> input_tables;
  for (const auto& p : params) {
    if (p.first == "input_table") {
      input_tables.emplace_back(p.second);
    }
  }

  String program_source;
  if (!URI::getParam(params, "program_source", &program_source)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?program_source=... parameter");
    return;
  }

  String method_name;
  if (!URI::getParam(params, "method_name", &method_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?method_name=... parameter");
    return;
  }

  auto result_id = service_->reduceTables(
      session,
      input_tables,
      program_source,
      method_name);

  if (result_id.isEmpty()) {
    res->setStatus(http::kStatusNoContent);
  } else {
    res->setStatus(http::kStatusCreated);
    res->addBody(result_id.get().toString());
  }
}

void MapReduceAPIServlet::executeSaveToTableTask(
    const AnalyticsSession& session,
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI::ParamList params;
  URI::parseQueryString(req->body().toString(), &params);

  String result_id;
  if (!URI::getParam(params, "result_id", &result_id)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?result_id=... parameter");
    return;
  }

  String table_name;
  if (!URI::getParam(params, "table_name", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table_name=... parameter");
    return;
  }

  String partition;
  if (!URI::getParam(params, "partition", &partition)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?partition=... parameter");
    return;
  }

  bool saved = service_->saveLocalResultToTable(
      session,
      table_name,
      SHA1Hash::fromHexString(partition),
      SHA1Hash::fromHexString(result_id));

  if (saved) {
    res->setStatus(http::kStatusCreated);
  } else {
    res->setStatus(http::kStatusNoContent);
  }
}

void MapReduceAPIServlet::executeSaveToTablePartitionTask(
    const AnalyticsSession& session,
    const URI& uri,
    const http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI::ParamList params;
  URI::parseQueryString(req->body().toString(), &params);

  Vector<String> input_tables;
  for (const auto& p : params) {
    if (p.first == "input_table") {
      input_tables.emplace_back(p.second);
    }
  }

  String table_name;
  if (!URI::getParam(params, "table_name", &table_name)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?table_name=... parameter");
    return;
  }

  String partition;
  if (!URI::getParam(params, "partition", &partition)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing ?partition=... parameter");
    return;
  }

  bool saved = service_->saveRemoteResultsToTable(
      session,
      table_name,
      SHA1Hash::fromHexString(partition),
      input_tables);

  if (saved) {
    res->setStatus(http::kStatusCreated);
  } else {
    res->setStatus(http::kStatusNoContent);
  }
}


void MapReduceAPIServlet::executeMapReduceScript(
    const AnalyticsSession& session,
    const URI& uri,
    http::HTTPRequestStream* req_stream,
    http::HTTPResponseStream* res_stream) {
  req_stream->readBody();

  http::HTTPSSEStream sse_stream(req_stream, res_stream);
  sse_stream.start();

  auto job_spec = mkRef(new MapReduceJobSpec{});
  job_spec->program_source = req_stream->request().body().toString();

  job_spec->onProgress([this, &sse_stream] (const MapReduceJobStatus& s) {
    if (sse_stream.isClosed()) {
      stx::logDebug("z1.mapreduce", "Aborting Job...");
      return;
    }

    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("status");
    json.addString("running");
    json.addComma();
    json.addObjectEntry("progress");
    json.addFloat(s.num_tasks_total > 0
        ? s.num_tasks_completed / (double) s.num_tasks_total
        : 0);
    json.addComma();
    json.addObjectEntry("num_tasks_total");
    json.addInteger(s.num_tasks_total);
    json.addComma();
    json.addObjectEntry("num_tasks_completed");
    json.addInteger(s.num_tasks_completed);
    json.addComma();
    json.addObjectEntry("num_tasks_running");
    json.addInteger(s.num_tasks_running);
    json.endObject();

    sse_stream.sendEvent(buf, Some(String("status")));
  });

  job_spec->onResult([this, &sse_stream] (
      const String& key,
      const String& value) {
    if (sse_stream.isClosed()) {
      stx::logDebug("z1.mapreduce", "Aborting Job...");
      return;
    }

    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("key");
    json.addString(key);
    json.addComma();
    json.addObjectEntry("value");
    json.addString(value);
    json.endObject();

    sse_stream.sendEvent(buf, Some(String("result")));
  });

  bool error = false;
  try {
    service_->executeScript(session, job_spec);
  } catch (const StandardException& e) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("status");
    json.addString("error");
    json.addComma();
    json.addObjectEntry("error");
    json.addString(e.what());
    json.endObject();

    sse_stream.sendEvent(buf, Some(String("status")));
    error = true;
  }

  if (!error) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
    json.beginObject();
    json.addObjectEntry("status");
    json.addString("success");
    json.endObject();

    sse_stream.sendEvent(buf, Some(String("status")));
  }

  sse_stream.finish();
}

void MapReduceAPIServlet::fetchResult(
    const AnalyticsSession& session,
    const String& result_id,
    http::HTTPRequestStream* req_stream,
    http::HTTPResponseStream* res_stream) {
  http::HTTPResponse res;
  res.populateFromRequest(req_stream->request());
  req_stream->readBody();

  URI uri(req_stream->request().uri());
  const auto& params = uri.queryParams();

  auto filename = service_->getResultFilename(
      SHA1Hash::fromHexString(result_id));

  if (filename.isEmpty()) {
    res.setStatus(http::kStatusNotFound);
    res_stream->writeResponse(res);
    return;
  }

  size_t sample_mod = 0;
  size_t sample_idx = 0;
  String sample_str;
  if (URI::getParam(params, "sample", &sample_str)) {
    auto parts = StringUtil::split(sample_str, ":");

    if (parts.size() != 2) {
      res.setStatus(stx::http::kStatusBadRequest);
      res.addBody("invalid ?sample=... parameter, format is <mod>:<idx>");
      res_stream->writeResponse(res);
      return;
    }

    sample_mod = std::stoull(parts[0]);
    sample_idx = std::stoull(parts[1]);
  }

  res.setStatus(http::kStatusOK);
  res.addHeader("Content-Type", "application/octet-stream");
  res.addHeader("Connection", "close");
  res_stream->startResponse(res);

  sstable::SSTableReader reader(filename.get());
  auto cursor = reader.getCursor();

  while (cursor->valid()) {
    void* key;
    size_t key_size;
    cursor->getKey((void**) &key, &key_size);

    FNV<uint64_t> fnv;
    if (sample_mod == 0 ||
        (fnv.hash(key, key_size) % sample_mod) == sample_idx) {
      void* data;
      size_t data_size;
      cursor->getData(&data, &data_size);

      util::BinaryMessageWriter buf;
      buf.appendUInt32(key_size);
      buf.appendUInt32(data_size);
      buf.append(key, key_size);
      buf.append(data, data_size);
      res_stream->writeBodyChunk(Buffer(buf.data(), buf.size()));
      res_stream->waitForReader();
    }

    if (!cursor->next()) {
      break;
    }
  }

  util::BinaryMessageWriter buf;
  buf.appendUInt32(0);
  buf.appendUInt32(0);
  res_stream->writeBodyChunk(Buffer(buf.data(), buf.size()));

  res_stream->finishResponse();
}

}
