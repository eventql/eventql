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
#include <algorithm>

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
    auto js_ctx = mkRef(new JavaScriptContext(session.customer(), tsdb_));
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
      tsdb_,
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
    const String& map_fn,
    const String& globals,
    const String& params) {
  auto table = pmap_->findTable(
      session.customer(),
      table_name);

  if (table.isEmpty()) {
    RAISEF(
        kNotFoundError,
        "table not found: $0/$1",
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

  auto output_id = SHA1::compute(
      StringUtil::format(
          "$0~$1~$2~$3~$4~$5",
          session.customer(),
          table_name,
          partition_key.toString(),
          reader->version().toString(),
          SHA1::compute(map_fn).toString()));

  auto output_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("mr-shard-$0.sst", output_id.toString()));

  if (FileUtil::exists(output_path)) {
    return Some(output_id);
  }

  logDebug(
      "z1.mapreduce",
      "Executing map shard; partition=$0/$1/$2 output=$3",
      session.customer(),
      table_name,
      partition_key.toString(),
      output_id.toString());

  auto js_ctx = mkRef(new JavaScriptContext(session.customer(), tsdb_));
  js_ctx->loadClosure(map_fn, globals, params);

  auto writer = sstable::SSTableWriter::create(output_path, nullptr, 0);

  reader->fetchRecords(
      [&schema, &js_ctx, &writer] (const msg::MessageObject& record) {
    Buffer json;
    json::JSONOutputStream jsons(BufferOutputStream::fromBuffer(&json));
    msg::JSONEncoder::encode(record, *schema, &jsons);

    Vector<Pair<String, String>> tuples;
    js_ctx->callMapFunction(json.toString(), &tuples);

    for (const auto& t : tuples) {
      writer->appendRow(t.first, t.second);
    }
  });

  return Some(output_id);
}

Option<SHA1Hash> MapReduceService::reduceTables(
    const AnalyticsSession& session,
    const Vector<String>& input_tables_ref,
    const String& reduce_fn,
    const String& globals,
    const String& params) {
  auto input_tables = input_tables_ref;
  std::sort(input_tables.begin(), input_tables.end());

  auto output_id = SHA1::compute(
      StringUtil::format(
          "$0~$1~$2",
          session.customer(),
          StringUtil::join(input_tables, "|"),
          SHA1::compute(reduce_fn).toString()));

  auto output_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("mr-shard-$0.sst", output_id.toString()));

  if (FileUtil::exists(output_path)) {
    return Some(output_id);
  }

  logDebug(
      "z1.mapreduce",
      "Executing reduce shard; customer=$0 input_tables=0/$1 output=$2",
      session.customer(),
      input_tables.size(),
      output_id.toString());

  std::random_shuffle(input_tables.begin(), input_tables.end());

  // FIXME MOST NAIVE IN MEMORY MERGE AHEAD !!
  HashMap<String, Vector<String>> groups;
  size_t num_input_tables_read = 0;
  size_t num_bytes_read = 0;
  for (const auto& input_table_url : input_tables) {
    auto api_token = auth_->encodeAuthToken(session);
    http::HTTPMessage::HeaderList auth_headers;
    auth_headers.emplace_back(
        "Authorization",
        StringUtil::format("Token $0", api_token));

    auto req = http::HTTPRequest::mkGet(input_table_url, auth_headers);
    MapReduceService::downloadResult(
        req,
        [&groups, &num_bytes_read] (
            const void* key,
            size_t key_len,
            const void* val,
            size_t val_len) {
      auto& lst = groups[String((const char*) key, key_len)];
      lst.emplace_back(String((const char*) val, val_len));
      num_bytes_read += key_len + val_len;
    });

    ++num_input_tables_read;
    logDebug(
      "z1.mapreduce",
      "Executing reduce shard; customer=$0 input_tables=$1/$2 output=$3 mem_used=$4MB",
      session.customer(),
      input_tables.size(),
      num_input_tables_read,
      output_id.toString(),
      num_bytes_read / 1024.0 / 1024.0);
  }

  if (groups.size() == 0) {
    return None<SHA1Hash>();
  }

  auto js_ctx = mkRef(new JavaScriptContext(session.customer(), tsdb_));
  js_ctx->loadClosure(reduce_fn, globals, params);

  auto writer = sstable::SSTableWriter::create(output_path, nullptr, 0);

  for (auto cur = groups.begin(); cur != groups.end(); ) {
    Vector<Pair<String, String>> tuples;
    js_ctx->callReduceFunction(cur->first, cur->second, &tuples);

    for (const auto& t : tuples) {
      writer->appendRow(t.first, t.second);
    }

    cur = groups.erase(cur);
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

void MapReduceService::downloadResult(
    const http::HTTPRequest& req,
    Function<void (const void*, size_t, const void*, size_t)> fn) {
  Buffer buffer;
  bool eos = false;
  auto handler = [&buffer, &eos, &fn] (const void* data, size_t size) {
    buffer.append(data, size);
    size_t consumed = 0;

    util::BinaryMessageReader reader(buffer.data(), buffer.size());
    while (reader.remaining() >= (sizeof(uint32_t) * 2)) {
      auto key_len = *reader.readUInt32();
      auto val_len = *reader.readUInt32();
      auto rec_len = key_len + val_len;

      if (rec_len > reader.remaining()) {
        break;
      }

      if (rec_len == 0) {
        eos = true;
      } else {
        auto key = reader.read(key_len);
        auto val = reader.read(val_len);
        fn(key, key_len, val, val_len);
      }

      consumed = reader.position();
    }

    Buffer remaining(
        (char*) buffer.data() + consumed,
        buffer.size() - consumed);
    buffer.clear();
    buffer.append(remaining);
  };

  auto handler_factory = [&handler] (
      const Promise<http::HTTPResponse> promise)
      -> http::HTTPResponseFuture* {
    return new http::StreamingResponseFuture(promise, handler);
  };

  http::HTTPClient http_client;
  auto res = http_client.executeRequest(req, handler_factory);
  handler(nullptr, 0);

  if (!eos) {
    RAISE(kRuntimeError, "unexpected EOF");
  }

  if (res.statusCode() != 200) {
    RAISEF(kRuntimeError, "received non-200 response for $0", req.uri());
  }
}

bool MapReduceService::saveLocalResultToTable(
    const AnalyticsSession& session,
    const String& table_name,
    const SHA1Hash& partition,
    const SHA1Hash& result_id) {
  auto table = pmap_->findTable(
      session.customer(),
      table_name);

  if (table.isEmpty()) {
    RAISEF(
        kNotFoundError,
        "table not found: $0/$1",
        session.customer(),
        table_name);
  }

  auto sstable_file = getResultFilename(result_id);
  if (sstable_file.isEmpty()) {
    RAISEF(
        kNotFoundError,
        "result not found: $0/$1",
        session.customer(),
        result_id.toString());
  }

  logDebug(
      "z1.mapreduce",
      "Saving result shard to table; result_id=$0 table=$1/$2/$3",
      result_id.toString(),
      session.customer(),
      table_name,
      partition.toString());

  auto schema = table.get()->schema();

  auto tmpfile_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("mr-shard-$0.cst", Random::singleton()->hex128()));

  {
    cstable::CSTableBuilder cstable(schema.get());
    sstable::SSTableReader sstable(sstable_file.get());
    auto cursor = sstable.getCursor();
    while (cursor->valid()) {
      void* data;
      size_t data_size;
      cursor->getData(&data, &data_size);

      auto json = json::parseJSON(String((const char*) data, data_size));
      msg::DynamicMessage msg(schema);
      msg.fromJSON(json.begin(), json.end());

      cstable.addRecord(msg.data());

      if (!cursor->next()) {
        break;
      }
    }

    cstable.write(tmpfile_path);
  }

  tsdb_->updatePartitionCSTable(
      session.customer(),
      table_name,
      partition,
      tmpfile_path,
      WallClock::unixMicros());

  return true;
}

bool MapReduceService::saveRemoteResultsToTable(
    const AnalyticsSession& session,
    const String& table_name,
    const SHA1Hash& partition,
    const Vector<String>& input_tables_ref) {
  auto input_tables = input_tables_ref;
  std::random_shuffle(input_tables.begin(), input_tables.end());

  auto table = pmap_->findTable(
      session.customer(),
      table_name);

  if (table.isEmpty()) {
    RAISEF(
        kNotFoundError,
        "table not found: $0/$1",
        session.customer(),
        table_name);
  }

  logDebug(
      "z1.mapreduce",
      "Saving results to table; input_tables=$0 table=$1/$2/$3",
      input_tables.size(),
      session.customer(),
      table_name,
      partition.toString());

  auto schema = table.get()->schema();

  auto tmpfile_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("mr-shard-$0.cst", Random::singleton()->hex128()));

  {
    cstable::CSTableBuilder cstable(schema.get());

    for (const auto& input_table_url : input_tables) {
      auto api_token = auth_->encodeAuthToken(session);
      http::HTTPMessage::HeaderList auth_headers;
      auth_headers.emplace_back(
          "Authorization",
          StringUtil::format("Token $0", api_token));

      auto req = http::HTTPRequest::mkGet(input_table_url, auth_headers);
      MapReduceService::downloadResult(
          req,
          [&cstable, &schema] (
              const void* key,
              size_t key_len,
              const void* val,
              size_t val_len) {
        auto json = json::parseJSON(String((const char*) val, val_len));
        msg::DynamicMessage msg(schema);
        msg.fromJSON(json.begin(), json.end());

        cstable.addRecord(msg.data());
      });
    }

    cstable.write(tmpfile_path);
  }

  tsdb_->updatePartitionCSTable(
      session.customer(),
      table_name,
      partition,
      tmpfile_path,
      WallClock::unixMicros());

  return true;
}

} // namespace zbase
