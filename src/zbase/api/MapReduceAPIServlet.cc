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
#include "stx/protobuf/msg.h"
#include "stx/io/BufferedOutputStream.h"
#include "zbase/api/MapReduceAPIServlet.h"
#include "zbase/mapreduce/MapReduceTask.h"

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
  const auto& params = uri.queryParams();

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

  auto shard_id = service_->mapPartition(
      session,
      table_name,
      SHA1Hash::fromHexString(partition_key),
      program_source,
      method_name);

  if (shard_id.isEmpty()) {
    res->setStatus(http::kStatusNoContent);
  } else {
    res->setStatus(http::kStatusCreated);
    res->addBody(shard_id.get().toString());
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
  }

  {
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
  req_stream->readBody();

  auto filename = service_->getResultFilename(
      SHA1Hash::fromHexString(result_id));

  if (filename.isEmpty()) {
    http::HTTPResponse res;
    res.populateFromRequest(req_stream->request());
    res.setStatus(http::kStatusNotFound);
    res_stream->writeResponse(res);
    return;
  }

  auto filesize = FileUtil::size(filename.get());
  auto file = File::openFile(filename.get(), File::O_READ);

  http::HTTPResponse res;
  res.populateFromRequest(req_stream->request());
  res.setStatus(http::kStatusOK);
  res.addHeader("Content-Length", StringUtil::toString(filesize));
  res_stream->startResponse(res);

  Buffer buf(1024 * 1024 * 4);
  for (;;) {
    auto chunk = file.read(buf.data(), buf.size());
    if (chunk == 0) {
      break;
    }

    res_stream->writeBodyChunk(buf.data(), chunk);
    res_stream->waitForReader();
  }

  res_stream->finishResponse();
}

}
