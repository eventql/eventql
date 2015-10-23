/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/api/MapReduceService.h"
#include "zbase/mapreduce/MapReduceTask.h"
#include "zbase/mapreduce/MapReduceTaskBuilder.h"
#include "zbase/mapreduce/MapReduceScheduler.h"
#include "stx/protobuf/MessageDecoder.h"
#include "stx/protobuf/JSONEncoder.h"
#include "stx/http/HTTPFileDownload.h"
#include "sstable/SSTableWriter.h"
#include "sstable/sstablereader.h"

using namespace stx;

namespace zbase {

MapReduceService::MapReduceService(
    ConfigDirectory* cdir,
    AnalyticsAuth* auth,
    zbase::TSDBService* tsdb,
    zbase::PartitionMap* pmap,
    zbase::ReplicationScheme* repl,
    JSRuntime* js_runtime,
    const String& cachedir) :
    cdir_(cdir),
    auth_(auth),
    tsdb_(tsdb),
    pmap_(pmap),
    repl_(repl),
    js_runtime_(js_runtime),
    cachedir_(cachedir) {}

void MapReduceService::executeScript(
    const AnalyticsSession& session,
    RefPtr<MapReduceJobSpec> job) {
  logDebug(
      "z1.mapreduce",
      "Launching mapreduce job; customer=$0",
      session.customer());

  json::JSONObject job_json;
  {
    auto js_ctx = mkRef(new JavaScriptContext());
    js_ctx->loadProgram(job->program_source);

    auto job_json_str = js_ctx->getMapReduceJobJSON();
    if (job_json_str.isEmpty()) {
      return;
    }

    job_json = json::parseJSON(job_json_str.get());
  }

  MapReduceTaskBuilder task_builder(
      session,
      job,
      auth_,
      pmap_,
      repl_,
      cachedir_);

  auto task_shards = task_builder.fromJSON(job_json.begin(), job_json.end());

  auto scheduler = mkRef(
      new MapReduceScheduler(
          session,
          job,
          task_shards,
          &tpool_,
          auth_,
          cachedir_));

  scheduler->execute();
}

Option<SHA1Hash> MapReduceService::mapPartition(
    const AnalyticsSession& session,
    const String& table_name,
    const SHA1Hash& partition_key,
    const String& program_source,
    const String& method_name) {
  auto output_id = Random::singleton()->sha1(); // FIXME
  logDebug(
      "z1.mapreduce",
      "Executing map shard; partition=$0/$1/$2 output=$3",
      session.customer(),
      table_name,
      partition_key.toString(),
      output_id.toString());

  auto table = pmap_->findTable(
      session.customer(),
      table_name);

  if (table.isEmpty()) {
    RAISEF(
        kNotFoundError,
        "table not found: $0/$1/$2",
        session.customer(),
        table_name);
  }

  auto partition = pmap_->findPartition(
      session.customer(),
      table_name,
      partition_key);

  if (partition.isEmpty()) {
    return None<SHA1Hash>();
  }

  auto schema = table.get()->schema();
  auto reader = partition.get()->getReader();

  auto js_ctx = mkRef(new JavaScriptContext());
  js_ctx->loadProgram(program_source);

  auto output_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("mr-shard-$0.sst", output_id.toString()));

  auto writer = sstable::SSTableWriter::create(output_path, nullptr, 0);

  iputs("starting  fetch...", 1);
  reader->fetchRecords(
      [
        &schema,
        &js_ctx,
        &method_name,
        &writer
      ] (
        const SHA1Hash& record_id,
        const msg::MessageObject& record
      ) {
    Buffer json;
    json::JSONOutputStream jsons(BufferOutputStream::fromBuffer(&json));
    msg::JSONEncoder::encode(record, *schema, &jsons);

    Vector<Pair<String, String>> tuples;
    js_ctx->callMapFunction(method_name, json.toString(), &tuples);

    for (const auto& t : tuples) {
      writer->appendRow(t.first, t.second);
    }
  });

  return Some(output_id);
}

Option<SHA1Hash> MapReduceService::reduceTables(
    const AnalyticsSession& session,
    const Vector<String>& input_tables,
    const String& program_source,
    const String& method_name) {
  auto output_id = Random::singleton()->sha1(); // FIXME
  logDebug(
      "z1.mapreduce",
      "Executing reduce shard; customer=$0 input_tables=$1 output=$2",
      session.customer(),
      input_tables.size(),
      output_id.toString());

  // FIXME MOST NAIVE IMPLEMENTATION AHEAD !!
  HashMap<String, Vector<String>> groups;

  for (const auto& input_table_url : input_tables) {
    auto result_path = FileUtil::joinPaths(
        cachedir_,
        StringUtil::format("mr-result-$0", Random::singleton()->sha1().toString()));

    auto api_token = auth_->encodeAuthToken(session);

    http::HTTPMessage::HeaderList auth_headers;
    auth_headers.emplace_back(
        "Authorization",
        StringUtil::format("Token $0", api_token));

    auto req = http::HTTPRequest::mkGet(input_table_url, auth_headers);

    http::HTTPClient http_client;
    http::HTTPFileDownload download(req, result_path);
    auto res = download.download(&http_client);
    if (res.statusCode() != 200) {
      RAISEF(kRuntimeError, "received non-201 response for $0", input_table_url);
    }

    sstable::SSTableReader reader(result_path);
    auto cursor = reader.getCursor();

    while (cursor->valid()) {
      auto& lst = groups[cursor->getKeyString()];
      lst.emplace_back(cursor->getDataString());

      if (!cursor->next()) {
        break;
      }
    }
  }

  if (groups.size() == 0) {
    return None<SHA1Hash>();
  }

  auto js_ctx = mkRef(new JavaScriptContext());
  js_ctx->loadProgram(program_source);

  auto output_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("mr-shard-$0.sst", output_id.toString()));

  auto writer = sstable::SSTableWriter::create(output_path, nullptr, 0);

  for (const auto& g : groups) {
    Vector<Pair<String, String>> tuples;
    js_ctx->callReduceFunction(method_name, g.first, g.second, &tuples);

    for (const auto& t : tuples) {
      writer->appendRow(t.first, t.second);
    }
  }

  return Some(output_id);
}

Option<String> MapReduceService::getResultFilename(
    const SHA1Hash& result_id) {
  auto result_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("mr-shard-$0.sst", result_id.toString()));

  if (FileUtil::exists(result_path)) {
    return Some(result_path);
  } else {
    return None<String>();
  }
}

} // namespace zbase
