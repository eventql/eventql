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
#include "sstable/SSTableWriter.h"

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

  auto task = task_builder.fromJSON(job_json.begin(), job_json.end());

  MapReduceShardList task_shards;
  task->build(&task_shards);

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

  reader->fetchRecords([
      &schema,
      &js_ctx,
      &method_name,
      &writer] (
      const Buffer& record) {
    msg::MessageObject msgobj;
    msg::MessageDecoder::decode(record, *schema, &msgobj);
    Buffer msgjson;
    json::JSONOutputStream msgjsons(BufferOutputStream::fromBuffer(&msgjson));
    msg::JSONEncoder::encode(msgobj, *schema, &msgjsons);

    Vector<Pair<String, String>> tuples;
    js_ctx->callMapFunction(method_name, msgjson.toString(), &tuples);

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

  return None<SHA1Hash>();
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
