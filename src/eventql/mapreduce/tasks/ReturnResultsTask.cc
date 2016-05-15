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

