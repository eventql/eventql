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
#ifndef _FNORD_TSDB_STREAMCHUNK_H
#define _FNORD_TSDB_STREAMCHUNK_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/option.h>
#include <eventql/util/UnixTime.h>
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/util/http/httpconnectionpool.h>
#include <eventql/core/ServerConfig.h>
#include <eventql/core/Table.h>
#include <eventql/core/RecordRef.h>
#include <eventql/core/PartitionInfo.pb.h>
#include <eventql/core/PartitionSnapshot.h>
#include <eventql/core/PartitionWriter.h>
#include <eventql/core/PartitionReader.h>
#include <eventql/core/ReplicationScheme.h>
#include <eventql/io/cstable/CSTableReader.h>

#include "eventql/eventql.h"

namespace eventql {
class Table;
class PartitionReplication;

using PartitionKey =
    std::tuple<
        String,     // namespace
        String,     // table
        SHA1Hash>;  // partition

class Partition : public RefCounted {
public:

  static RefPtr<Partition> create(
      const String& tsdb_namespace,
      RefPtr<Table> table,
      const SHA1Hash& partition_key,
      ServerConfig* cfg);

  static RefPtr<Partition> reopen(
      const String& tsdb_namespace,
      RefPtr<Table> table,
      const SHA1Hash& partition_key,
      ServerConfig* cfg);

  Partition(
      ServerConfig* cfg,
      RefPtr<PartitionSnapshot> snap,
      RefPtr<Table> table);

  ~Partition();

  SHA1Hash uuid() const;

  RefPtr<PartitionWriter> getWriter();
  RefPtr<PartitionReader> getReader();
  RefPtr<PartitionSnapshot> getSnapshot();
  RefPtr<Table> getTable();
  PartitionInfo getInfo() const;

  RefPtr<PartitionReplication> getReplicationStrategy(
      RefPtr<ReplicationScheme> repl_scheme,
      http::HTTPConnectionPool* http);

  String getRelativePath() const;
  String getAbsolutePath() const;

protected:

  bool upgradeToLSMv2() const;

  ServerConfig* cfg_;
  PartitionSnapshotRef head_;
  RefPtr<Table> table_;
  RefPtr<PartitionWriter> writer_;
  std::mutex writer_lock_;
};

}
#endif
