/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/mapreduce/MapReduceTaskBuilder.h"
#include "zbase/mapreduce/tasks/MapTableTask.h"
#include "zbase/mapreduce/tasks/ReduceTask.h"
#include "zbase/mapreduce/tasks/ReturnResultsTask.h"
#include "zbase/AnalyticsAuth.h"
#include "zbase/CustomerConfig.h"
#include "zbase/ConfigDirectory.h"

using namespace stx;

namespace zbase {

MapReduceTaskBuilder::MapReduceTaskBuilder(
    const AnalyticsSession& session,
    RefPtr<MapReduceJobSpec> job_spec,
    AnalyticsAuth* auth,
    zbase::PartitionMap* pmap,
    zbase::ReplicationScheme* repl,
    const String& cachedir) :
    session_(session),
    job_spec_(job_spec),
    auth_(auth),
    pmap_(pmap),
    repl_(repl),
    cachedir_(cachedir) {}

RefPtr<MapReduceTask> MapReduceTaskBuilder::fromJSON(
    const json::JSONObject::const_iterator& begin,
    const json::JSONObject::const_iterator& end) {
  auto op = json::objectGetString(begin, end, "op");
  if (op.isEmpty()) {
    RAISE(kRuntimeError, "illegal job definition: missing op field");
  }

  if (op.get() == "return_results") {
    return returnResultsTaskFromJSON(begin, end);
  }

  if (op.get() == "map_table") {
    return mapTableTaskFromJSON(begin, end);
  }

  if (op.get() == "reduce") {
    return reduceTaskFromJSON(begin, end);
  }

  RAISEF(kRuntimeError, "unknown operation: $0", op.get());
}

RefPtr<MapReduceTask> MapReduceTaskBuilder::mapTableTaskFromJSON(
    const json::JSONObject::const_iterator& begin,
    const json::JSONObject::const_iterator& end) {
  TSDBTableRef table_ref;

  auto table_name = json::objectGetString(begin, end, "table_name");
  if (table_name.isEmpty()) {
    RAISE(kRuntimeError, "missing field: table_name");
  } else {
    table_ref.table_key = table_name.get();
  }

  auto from = json::objectGetUInt64(begin, end, "from");
  if (!from.isEmpty()) {
    table_ref.timerange_begin = UnixTime(from.get());
  }

  auto until = json::objectGetUInt64(begin, end, "until");
  if (!until.isEmpty()) {
    table_ref.timerange_limit = UnixTime(until.get());
  }

  auto method_name = json::objectGetString(begin, end, "method_name");
  if (method_name.isEmpty()) {
    RAISE(kRuntimeError, "missing field: method_name");
  }

  return new MapTableTask(
      session_,
      job_spec_,
      table_ref,
      method_name.get(),
      auth_,
      pmap_,
      repl_);
}

RefPtr<MapReduceTask> MapReduceTaskBuilder::reduceTaskFromJSON(
    const json::JSONObject::const_iterator& begin,
    const json::JSONObject::const_iterator& end) {
  auto src_begin = json::objectLookup(begin, end, "sources");
  if (src_begin == end) {
    RAISE(kRuntimeError, "missing field: sources");
  }

  auto method_name = json::objectGetString(begin, end, "method_name");
  if (method_name.isEmpty()) {
    RAISE(kRuntimeError, "missing field: method_name");
  }

  auto num_shards = json::objectGetUInt64(begin, end, "num_shards");
  if (num_shards.isEmpty()) {
    RAISE(kRuntimeError, "missing field: num_shards");
  }

  Vector<RefPtr<MapReduceTask>> sources;
  auto nsrc_begin = json::arrayLength(src_begin, end);
  for (size_t i = 0; i < nsrc_begin; ++i) {
    auto src = json::arrayLookup(src_begin, end, i); // O(N^2) but who cares...
    sources.emplace_back(fromJSON(src, src + src->size));
  }

  return new ReduceTask(
      session_,
      job_spec_,
      method_name.get(),
      sources,
      num_shards.get(),
      auth_,
      repl_);
}

RefPtr<MapReduceTask> MapReduceTaskBuilder::returnResultsTaskFromJSON(
    const json::JSONObject::const_iterator& begin,
    const json::JSONObject::const_iterator& end) {
  auto src_begin = json::objectLookup(begin, end, "sources");
  if (src_begin == end) {
    RAISE(kRuntimeError, "missing field: sources");
  }

  Vector<RefPtr<MapReduceTask>> sources;
  auto nsrc_begin = json::arrayLength(src_begin, end);
  for (size_t i = 0; i < nsrc_begin; ++i) {
    auto src = json::arrayLookup(src_begin, end, i); // O(N^2) but who cares...
    sources.emplace_back(fromJSON(src, src + src->size));
  }

  return new ReturnResultsTask(sources);
}

} // namespace zbase

