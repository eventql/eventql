/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/mapreduce/tasks/ReturnResultsTask.h"
#include "eventql/mapreduce/MapReduceScheduler.h"
#include "eventql/infra/sstable/sstablereader.h"
#include "eventql/JavaScriptContext.h"

using namespace stx;

namespace zbase {

ReturnResultsTask::ReturnResultsTask(
    Vector<RefPtr<MapReduceTask>> sources,
    MapReduceShardList* shards,
    const AnalyticsSession& session,
    const String& serialize_fn,
    const String& globals,
    const String& params) :
    sources_(sources),
    session_(session),
    serialize_fn_(serialize_fn),
    globals_(globals),
    params_(params) {
  Vector<size_t> input;

  for (const auto& src : sources_) {
    auto src_indexes = src->shards();
    input.insert(input.end(), src_indexes.begin(), src_indexes.end());
  }

  auto shard = mkRef(new MapReduceTaskShard());
  shard->task = this;
  shard->dependencies = input;

  addShard(shard, shards);
}

Option<MapReduceShardResult> ReturnResultsTask::execute(
    RefPtr<MapReduceTaskShard> shard,
    RefPtr<MapReduceScheduler> job) {
  if (serialize_fn_.empty()) {
    for (const auto& input : shard->dependencies) {
      job->downloadResult(
          input,
          [&job] (
              const void* key,
              size_t key_len,
              const void* val,
              size_t val_len) {
        Buffer buf;
        json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));
        json.beginObject();
        json.addObjectEntry("key");
        json.addString(String((const char*) key, key_len));
        json.addComma();
        json.addObjectEntry("value");
        json.addString(String((const char*) val, val_len));
        json.endObject();

        job->sendResult(buf.toString());
      });
    }
  } else {
    auto js_ctx = mkRef(new JavaScriptContext(
        session_.customer(),
        job->jobSpec(),
        nullptr,
        nullptr,
        nullptr));

    js_ctx->loadClosure(serialize_fn_, globals_, params_);

    for (const auto& input : shard->dependencies) {
      job->downloadResult(
          input,
          [&job, &js_ctx] (
              const void* key,
              size_t key_len,
              const void* val,
              size_t val_len) {
        auto result = js_ctx->callSerializeFunction(
            String((const char*) key, key_len),
            String((const char*) val, val_len));

        if (!result.empty()) {
          job->sendResult(result);
        }
      });
    }
  }

  return None<MapReduceShardResult>();
}

} // namespace zbase

