/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_TSDBNODE_H
#define _FNORD_TSDB_TSDBNODE_H
#include <stx/stdtypes.h>
#include <stx/random.h>
#include <stx/option.h>
#include <stx/thread/queue.h>
#include <stx/mdb/MDB.h>
#include <tsdb/TableConfig.pb.h>
#include <tsdb/Partition.h>
#include <tsdb/TSDBNodeConfig.pb.h>
#include <tsdb/TSDBTableInfo.h>
#include <tsdb/PartitionInfo.pb.h>
#include <tsdb/RecordEnvelope.pb.h>
#include <tsdb/PartitionMap.h>

using namespace stx;

namespace tsdb {

class TSDBService {
public:

  TSDBService(PartitionMap* pmap);

  void createTable(const TableDefinition& config);

  void insertRecords(const RecordEnvelopeList& records);

  void insertRecord(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      const SHA1Hash& record_id,
      const Buffer& record);

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

  Vector<String> listPartitions(
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
  PartitionMap* pmap_;
};

} // namespace tdsb

#endif
