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
#pragma once
#include "eventql/util/stdtypes.h"
#include "eventql/mapreduce/MapReduceTask.h"
#include "eventql/db/table_service.h"
#include "eventql/auth/internal_auth.h"

#include "eventql/eventql.h"

namespace eventql {

struct SaveToTableTaskShard : public MapReduceTaskShard {
  SHA1Hash partition;
};

class SaveToTableTask : public MapReduceTask {
public:

  SaveToTableTask(
      Session* session,
      const String& table_name,
      Vector<RefPtr<MapReduceTask>> sources,
      MapReduceShardList* shards,
      InternalAuth* auth,
      TableService* tsdb);

  Option<MapReduceShardResult> execute(
      RefPtr<MapReduceTaskShard> shard,
      RefPtr<MapReduceScheduler> job) override;

protected:
  Session* session_;
  String table_name_;
  Vector<RefPtr<MapReduceTask>> sources_;
  InternalAuth* auth_;
};

} // namespace eventql


