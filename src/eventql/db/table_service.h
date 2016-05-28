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
#ifndef _FNORD_TSDB_TSDBNODE_H
#define _FNORD_TSDB_TSDBNODE_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/random.h>
#include <eventql/util/option.h>
#include <eventql/util/protobuf/DynamicMessage.h>
#include <eventql/util/thread/queue.h>
#include <eventql/util/thread/eventloop.h>
#include <eventql/util/mdb/MDB.h>
#include <eventql/db/TableConfig.pb.h>
#include <eventql/db/Partition.h>
#include <eventql/db/TSDBNodeConfig.pb.h>
#include <eventql/db/TSDBTableInfo.h>
#include <eventql/db/PartitionInfo.pb.h>
#include <eventql/db/RecordEnvelope.pb.h>
#include <eventql/db/partition_map.h>
#include <eventql/db/TimeWindowPartitioner.h>
#include <eventql/config/config_directory.h>

#include "eventql/eventql.h"

namespace eventql {

enum class InsertFlags : uint64_t {
  REPLICATED_WRITE = 1,
  SYNC_COMMIT = 2
};

class TableService {
public:

  TableService(
      ConfigDirectory* cdir,
      PartitionMap* pmap,
      ReplicationScheme* repl,
      thread::EventLoop* ev,
      http::HTTPClientStats* http_stats);

  Status createTable(
      const String& db_namespace,
      const String& table_name,
      const msg::MessageSchema& schema,
      Vector<String> primary_key);

  void listTables(
      const String& tsdb_namespace,
      Function<void (const TSDBTableInfo& table)> fn) const;

  void listTablesReverse(
      const String& tsdb_namespace,
      Function<void (const TSDBTableInfo& table)> fn) const;

  void insertRecords(
      const RecordEnvelopeList& records,
      uint64_t flags = 0);

  void insertRecords(
      const Vector<RecordEnvelope>& records,
      uint64_t flags = 0);

  void insertRecords(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      const Vector<RecordRef>& records,
      uint64_t flags = 0);

  void insertRecord(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      const SHA1Hash& record_id,
      uint64_t record_version,
      const Buffer& record,
      uint64_t flags = 0);

  void insertRecord(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& record_id,
      uint64_t record_version,
      const json::JSONObject::const_iterator& data_begin,
      const json::JSONObject::const_iterator& data_end,
      uint64_t flags = 0);

  void insertRecord(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& record_id,
      uint64_t record_version,
      const msg::DynamicMessage& data,
      uint64_t flags = 0);

  void compactPartition(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key);

  void updatePartitionCSTable(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      const String& tmpfile_path,
      uint64_t version);

  void fetchPartition(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      Function<void (const Buffer& record)> fn);

  void fetchPartitionWithSampling(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      size_t sample_modulo,
      size_t sample_index,
      Function<void (const Buffer& record)> fn);

  Vector<TimeseriesPartition> listPartitions(
      const String& tsdb_namespace,
      const String& table_key,
      const UnixTime& from,
      const UnixTime& until);

  Option<PartitionInfo> partitionInfo(
      const String& tsdb_namespace,
      const String& table_key,
      const SHA1Hash& partition_key);

  Option<RefPtr<msg::MessageSchema>> tableSchema(
      const String& tsdb_namespace,
      const String& table_key);

  Option<TableDefinition> tableConfig(
      const String& tsdb_namespace,
      const String& table_key);

  Option<RefPtr<TablePartitioner>> tablePartitioner(
      const String& tsdb_namespace,
      const String& table_key);

protected:

  void insertRecordsLocal(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      const Vector<RecordRef>& records,
      uint64_t flags);

  void insertRecordsRemote(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      const Vector<RecordRef>& records,
      uint64_t flags,
      const ReplicaRef& host);

  ConfigDirectory* cdir_;
  PartitionMap* pmap_;
  ReplicationScheme* repl_;
  http::HTTPConnectionPool http_;
};

} // namespace tdsb

#endif
