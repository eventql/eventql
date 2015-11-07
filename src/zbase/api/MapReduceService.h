/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/thread/threadpool.h"
#include "zbase/core/TSDBService.h"
#include "zbase/AnalyticsAuth.h"
#include "zbase/CustomerConfig.h"
#include "zbase/ConfigDirectory.h"
#include "zbase/JavaScriptContext.h"
#include "zbase/mapreduce/MapReduceTask.h"

using namespace stx;

namespace zbase {

class MapReduceService {
public:

  MapReduceService(
      ConfigDirectory* cdir,
      AnalyticsAuth* auth,
      zbase::TSDBService* tsdb,
      zbase::PartitionMap* pmap,
      zbase::ReplicationScheme* repl,
      JSRuntime* js_runtime,
      const String& cachedir);

  void executeScript(
      const AnalyticsSession& session,
      RefPtr<MapReduceJobSpec> job,
      const String& program_source);

  Option<SHA1Hash> mapPartition(
      const AnalyticsSession& session,
      RefPtr<MapReduceJobSpec> job,
      const String& table_name,
      const SHA1Hash& partition_key,
      const String& map_fn,
      const String& globals,
      const String& params);

  Option<SHA1Hash> reduceTables(
      const AnalyticsSession& session,
      const Vector<String>& input_tables,
      const String& reduce_fn,
      const String& globals,
      const String& params);

  Option<String> getResultFilename(
      const SHA1Hash& result_id);

  bool saveLocalResultToTable(
      const AnalyticsSession& session,
      const String& table_name,
      const SHA1Hash& partition,
      const SHA1Hash& result_id);

  bool saveRemoteResultsToTable(
      const AnalyticsSession& session,
      const String& table_name,
      const SHA1Hash& partition,
      const Vector<String>& input_tables);

  static void downloadResult(
      const http::HTTPRequest& req,
      Function<void (const void*, size_t, const void*, size_t)> fn);

protected:
  ConfigDirectory* cdir_;
  AnalyticsAuth* auth_;
  zbase::TSDBService* tsdb_;
  zbase::PartitionMap* pmap_;
  zbase::ReplicationScheme* repl_;
  JSRuntime* js_runtime_;
  String cachedir_;
  thread::ThreadPool tpool_;
};

} // namespace zbase
