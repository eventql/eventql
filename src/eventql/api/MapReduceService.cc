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
#include "eventql/api/MapReduceService.h"
#include "eventql/mapreduce/MapReduceTask.h"
#include "eventql/mapreduce/MapReduceTaskBuilder.h"
#include "eventql/mapreduce/MapReduceScheduler.h"
#include "eventql/util/protobuf/MessageDecoder.h"
#include "eventql/util/protobuf/JSONEncoder.h"
#include "eventql/util/http/HTTPFileDownload.h"
#include "eventql/infra/sstable/SSTableWriter.h"
#include "eventql/infra/sstable/sstablereader.h"
#include "eventql/infra/cstable/CSTableWriter.h"
#include "eventql/infra/cstable/RecordShredder.h"
#include <algorithm>

#include "eventql/eventql.h"

namespace eventql {

MapReduceService::MapReduceService(
    ConfigDirectory* cdir,
    AnalyticsAuth* auth,
    eventql::TSDBService* tsdb,
    eventql::PartitionMap* pmap,
    eventql::ReplicationScheme* repl,
    JSRuntime* js_runtime,
    const String& cachedir) :
    cdir_(cdir),
    auth_(auth),
    tsdb_(tsdb),
    pmap_(pmap),
    repl_(repl),
    js_runtime_(js_runtime),
    cachedir_(cachedir),
    tpool_(
        thread::ThreadPoolOptions {
          .thread_name = Some(String("z1d-mapreduce"))
        },
        1) {}

void MapReduceService::executeScript(
    const AnalyticsSession& session,
    RefPtr<MapReduceJobSpec> job,
    const String& program_source) {
  logDebug(
      "z1.mapreduce",
      "Launching mapreduce job; customer=$0",
      session.customer());

  auto task_builder = mkRef(new MapReduceTaskBuilder(
      session,
      auth_,
      pmap_,
      repl_,
      tsdb_,
      cachedir_));


  auto scheduler = mkRef(new MapReduceScheduler(
      session,
      job,
      &tpool_,
      auth_,
      cachedir_));

  auto js_ctx = mkRef(new JavaScriptContext(
      session.customer(),
      job,
      tsdb_,
      task_builder,
      scheduler));

  js_ctx->loadProgram(program_source);
}

Option<SHA1Hash> MapReduceService::mapPartition(
    const AnalyticsSession& session,
    RefPtr<MapReduceJobSpec> job,
    const String& table_name,
    const SHA1Hash& partition_key,
    const String& map_fn,
    const String& globals,
    const String& params,
    const Set<String>& required_columns) {
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
          "$0~$1~$2~$3~$4~$5~$6~$7~$8",
          session.customer(),
          table_name,
          partition_key.toString(),
          reader->version().toString(),
          SHA1::compute(map_fn).toString(),
          SHA1::compute(globals).toString(),
          SHA1::compute(params).toString(),
          StringUtil::join(required_columns, ",")));

  auto output_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("mr-shard-$0.sst", output_id.toString()));

  if (FileUtil::exists(output_path)) {
    return Some(output_id);
  }

  auto output_path_tmp = StringUtil::format(
      "$0~$1",
      output_path,
      Random::singleton()->hex64());

  logDebug(
      "z1.mapreduce",
      "Executing map shard; partition=$0/$1/$2 output=$3",
      session.customer(),
      table_name,
      partition_key.toString(),
      output_id.toString());

  try {
    z1stats()->mapreduce_num_map_tasks.incr(1);

    auto js_ctx = mkRef(new JavaScriptContext(
        session.customer(),
        job,
        tsdb_,
        nullptr,
        nullptr));

    js_ctx->loadClosure(map_fn, globals, params);

    auto writer = sstable::SSTableWriter::create(output_path_tmp, nullptr, 0);

    reader->fetchRecords(
        required_columns,
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

  } catch (const Exception& e) {
    z1stats()->mapreduce_num_map_tasks.decr(1);
    throw e;
  }

  z1stats()->mapreduce_num_map_tasks.decr(1);

  FileUtil::mv(output_path_tmp, output_path);
  return Some(output_id);
}

Option<SHA1Hash> MapReduceService::reduceTables(
    const AnalyticsSession& session,
    RefPtr<MapReduceJobSpec> job,
    const Vector<String>& input_tables_ref,
    const String& reduce_fn,
    const String& globals,
    const String& params) {
  auto input_tables = input_tables_ref;
  std::sort(input_tables.begin(), input_tables.end());

  auto output_id = SHA1::compute(
      StringUtil::format(
          "$0~$1~$2~$3~$4",
          session.customer(),
          StringUtil::join(input_tables, "|"),
          SHA1::compute(reduce_fn).toString(),
          SHA1::compute(globals).toString(),
          SHA1::compute(params).toString()));

  auto output_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("mr-shard-$0.sst", output_id.toString()));

  if (FileUtil::exists(output_path)) {
    return Some(output_id);
  }

  auto output_path_tmp = StringUtil::format(
      "$0~$1",
      output_path,
      Random::singleton()->hex64());

  logDebug(
      "z1.mapreduce",
      "Executing reduce shard; customer=$0 input_tables=0/$1 output=$2",
      session.customer(),
      input_tables.size(),
      output_id.toString());

  std::random_shuffle(input_tables.begin(), input_tables.end());

  // FIXME MOST NAIVE IN MEMORY MERGE AHEAD !!
  size_t num_input_tables_read = 0;
  size_t num_bytes_read = 0;
  try {
    z1stats()->mapreduce_num_reduce_tasks.incr(1);

    HashMap<String, Vector<String>> groups;
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
        auto key_str = String((const char*) key, key_len);
        auto& lst = groups[key_str];
        lst.emplace_back(String((const char*) val, val_len));
        auto rec_size = key_len + val_len;
        num_bytes_read += rec_size;
        z1stats()->mapreduce_reduce_memory.incr(rec_size);
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
      z1stats()->mapreduce_reduce_memory.decr(num_bytes_read);
      z1stats()->mapreduce_num_reduce_tasks.decr(1);
      return None<SHA1Hash>();
    }

    auto js_ctx = mkRef(new JavaScriptContext(
        session.customer(),
        job,
        tsdb_,
        nullptr,
        nullptr));

    js_ctx->loadClosure(reduce_fn, globals, params);

    auto writer = sstable::SSTableWriter::create(output_path_tmp, nullptr, 0);

    for (const auto& cur : groups) {
      Vector<Pair<String, String>> tuples;
      js_ctx->callReduceFunction(cur.first, cur.second, &tuples);

      for (const auto& t : tuples) {
        writer->appendRow(t.first, t.second);
      }
    }
  } catch (const Exception& e) {
    z1stats()->mapreduce_reduce_memory.decr(num_bytes_read);
    z1stats()->mapreduce_num_reduce_tasks.decr(1);
    throw e;
  }

  z1stats()->mapreduce_reduce_memory.decr(num_bytes_read);
  z1stats()->mapreduce_num_reduce_tasks.decr(1);

  FileUtil::mv(output_path_tmp, output_path);
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

  http::HTTPClient http_client(&z1stats()->http_client_stats);
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
    auto cstable = cstable::CSTableWriter::createFile(
        tmpfile_path,
        cstable::BinaryFormatVersion::v0_1_0,
        cstable::TableSchema::fromProtobuf(*schema));

    cstable::RecordShredder shredder(cstable.get());

    sstable::SSTableReader sstable(sstable_file.get());
    auto cursor = sstable.getCursor();
    while (cursor->valid()) {
      void* data;
      size_t data_size;
      cursor->getData(&data, &data_size);

      auto json = json::parseJSON(String((const char*) data, data_size));
      msg::DynamicMessage msg(schema);
      msg.fromJSON(json.begin(), json.end());

      shredder.addRecordFromProtobuf(msg.data(), *schema);

      if (!cursor->next()) {
        break;
      }
    }

    cstable->commit();
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
    auto cstable = cstable::CSTableWriter::createFile(
        tmpfile_path,
        cstable::BinaryFormatVersion::v0_1_0,
        cstable::TableSchema::fromProtobuf(*schema));

    cstable::RecordShredder shredder(cstable.get());

    for (const auto& input_table_url : input_tables) {
      auto api_token = auth_->encodeAuthToken(session);
      http::HTTPMessage::HeaderList auth_headers;
      auth_headers.emplace_back(
          "Authorization",
          StringUtil::format("Token $0", api_token));

      auto req = http::HTTPRequest::mkGet(input_table_url, auth_headers);
      MapReduceService::downloadResult(
          req,
          [&shredder, &schema] (
              const void* key,
              size_t key_len,
              const void* val,
              size_t val_len) {
        auto json = json::parseJSON(String((const char*) val, val_len));
        msg::DynamicMessage msg(schema);
        msg.fromJSON(json.begin(), json.end());

        shredder.addRecordFromProtobuf(msg.data(), *schema);
      });
    }

    cstable->commit();
  }

  tsdb_->updatePartitionCSTable(
      session.customer(),
      table_name,
      partition,
      tmpfile_path,
      WallClock::unixMicros());

  return true;
}

} // namespace eventql
