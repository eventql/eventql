/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#pragma once
#include "eventql/eventql.h"
#include "eventql/util/thread/threadpool.h"
#include "eventql/db/table_service.h"
#include "eventql/auth/internal_auth.h"
#include "eventql/config/namespace_config.h"
#include "eventql/config/config_directory.h"
#include "eventql/mapreduce/runtime/javascript/javascript_context.h"
#include "eventql/mapreduce/mapreduce_task.h"
#include "eventql/server/session.h"

namespace eventql {

class MapReduceService {
public:

  MapReduceService(
      ConfigDirectory* cdir,
      InternalAuth* auth,
      eventql::TableService* tsdb,
      eventql::PartitionMap* pmap,
      const String& cachedir);

  void executeScript(
      Session* session,
      RefPtr<MapReduceJobSpec> job,
      const String& program_source);

  Option<SHA1Hash> mapPartition(
      Session* session,
      RefPtr<MapReduceJobSpec> job,
      const String& table_name,
      const SHA1Hash& partition_key,
      const String& map_fn,
      const String& globals,
      const String& params,
      const Set<String>& required_columns);

  Option<SHA1Hash> reduceTables(
      Session* session,
      RefPtr<MapReduceJobSpec> job,
      const Vector<String>& input_tables,
      const String& reduce_fn,
      const String& globals,
      const String& params);

  Option<String> getResultFilename(
      const SHA1Hash& result_id);

  bool saveResultToTable(
      Session* session,
      const String& table_name,
      const SHA1Hash& result_id);

  static void downloadResult(
      const http::HTTPRequest& req,
      Function<void (const void*, size_t, const void*, size_t)> fn);

protected:
  ConfigDirectory* cdir_;
  InternalAuth* auth_;
  eventql::TableService* tsdb_;
  eventql::PartitionMap* pmap_;
  String cachedir_;
  thread::ThreadPool tpool_;
};

} // namespace eventql
