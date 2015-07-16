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
#include <fnord/stdtypes.h>
#include <fnord/random.h>
#include <fnord/option.h>
#include <fnord/thread/queue.h>
#include <fnord/mdb/MDB.h>
#include <tsdb/TableConfig.pb.h>
#include <tsdb/Partition.h>
#include <tsdb/TSDBNodeRef.h>
#include <tsdb/CompactionWorker.h>
#include <tsdb/ReplicationWorker.h>
#include <tsdb/TSDBNodeConfig.pb.h>
#include <tsdb/TSDBTableInfo.h>
#include <tsdb/SQLEngine.h>
#include <tsdb/PartitionInfo.pb.h>
#include <tsdb/RecordEnvelope.pb.h>

using namespace fnord;

namespace tsdb {

class TSDBNode {
public:

  TSDBNode(
      const String& db_path,
      RefPtr<dproc::ReplicationScheme> replication_scheme,
      http::HTTPConnectionPool* http);

  void configure(const TSDBNodeConfig& config, const String& base_path);
  void configure(const TableConfig& config);

  TableConfig* configFor(
      const String& tsdb_namespace,
      const String& stream_key) const;

  Option<RefPtr<Partition>> findPartition(
      const String& tsdb_namespace,
      const String& stream_key,
      const SHA1Hash& partition_key);

  RefPtr<Partition> findOrCreatePartition(
      const String& tsdb_namespace,
      const String& stream_key,
      const SHA1Hash& partition_key);

  void listTables(
      const String& tsdb_namespace,
      Function<void (const TSDBTableInfo& table)> fn) const;

  Option<TSDBTableInfo> tableInfo(
      const String& tsdb_namespace,
      const String& table_key) const;

  void insertRecord(const RecordEnvelope& record);
  void insertRecords(const RecordEnvelopeList& records);

  void insertRecord(
      const String& tsdb_namespace,
      const String& stream_key,
      const SHA1Hash& partition_key,
      const SHA1Hash& record_id,
      const Buffer& record_data);

  void insertRecord(
      const String& tsdb_namespace,
      const String& stream_key,
      const UnixTime& time,
      const SHA1Hash& record_id,
      const Buffer& record_data);

  Vector<String> listPartitions(
      const String& stream_key,
      const UnixTime& from,
      const UnixTime& until);

  void fetchPartition(
      const String& tsdb_namespace,
      const String& stream_key,
      const SHA1Hash& partition_key,
      Function<void (const Buffer& record)> fn);

  void fetchPartitionWithSampling(
      const String& tsdb_namespace,
      const String& stream_key,
      const SHA1Hash& partition_key,
      size_t sample_modulo,
      size_t sample_index,
      Function<void (const Buffer& record)> fn);

  PartitionInfo fetchPartitionInfo(
      const String& tsdb_namespace,
      const String& stream_key,
      const SHA1Hash& partition_key);

  Buffer fetchDerivedDataset(
      const String& stream_key,
      const String& partition,
      const String& derived_dataset_name);

  const String& dbPath() const;

  void start(
      size_t num_comaction_threads = 8,
      size_t num_replication_threads = 4);

  void stop();

protected:

  void reopenPartitions();

  TSDBNodeRef noderef_;
  Vector<Pair<String, ScopedPtr<TableConfig>>> configs_;
  std::mutex mutex_;
  HashMap<String, RefPtr<Partition>> partitions_;
  Vector<RefPtr<CompactionWorker>> compaction_workers_;
  Vector<RefPtr<ReplicationWorker>> replication_workers_;
  msg::MessageSchemaRepository schemas_;
};

} // namespace tdsb

#endif
