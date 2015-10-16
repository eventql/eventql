/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/api/MapReduceService.h"

using namespace stx;

namespace zbase {

MapReduceService::MapReduceService(
    ConfigDirectory* cdir,
    AnalyticsAuth* auth,
    zbase::TSDBService* tsdb,
    zbase::PartitionMap* pmap,
    zbase::ReplicationScheme* repl,
    JSRuntime* js_runtime) :
    cdir_(cdir),
    auth_(auth),
    tsdb_(tsdb),
    pmap_(pmap),
    repl_(repl),
    js_runtime_(js_runtime) {}

void MapReduceService::mapPartition(
    const AnalyticsSession& session,
    const String& table_name,
    const SHA1Hash& partition_key) {
  logDebug(
      "z1.mapreduce",
      "Executing map shard; partition=$0/$1/$2 output=$3",
      session.customer(),
      table_name,
      partition_key.toString());

  auto partition = pmap_->findPartition(
      session.customer(),
      table_name,
      partition_key);

  if (partition.isEmpty()) {
    RAISEF(
        kNotFoundError,
        "partition not found: $0/$1/$2",
        session.customer(),
        table_name,
        partition_key.toString());
  }

  auto reader = partition.get()->getReader();

  auto js_ctx = mkRef(new JavaScriptContext());
  js_ctx->execute( "'hello'+'world, it is '+new Date()");

  reader->fetchRecords([] (const Buffer& record) {
    iputs("scan record: $0", record.size());
  });
}

} // namespace zbase
