/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/random.h>
#include <stx/option.h>
#include <stx/mdb/MDB.h>
#include <tsdb/TableConfig.pb.h>
#include <tsdb/Partition.h>
#include <tsdb/LazyPartition.h>
#include <tsdb/TSDBNodeConfig.pb.h>
#include <tsdb/TSDBTableInfo.h>
#include <tsdb/PartitionInfo.pb.h>
#include <tsdb/PartitionChangeNotification.h>
#include <tsdb/RecordEnvelope.pb.h>

using namespace stx;

namespace tsdb {

class PartitionMap {
public:
  PartitionMap(const String& db_path);

  void configureTable(const TableDefinition& config);
  void open();

  Option<RefPtr<Table>> findTable(
      const String& tsdb_namespace,
      const String& table_name) const;

  void listTables(
      const String& tsdb_namespace,
      Function<void (const TSDBTableInfo& table)> fn) const;

  Option<TSDBTableInfo> tableInfo(
      const String& tsdb_namespace,
      const String& table_key) const;

  Option<RefPtr<Partition>> findPartition(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key);

  RefPtr<Partition> findOrCreatePartition(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key);

  void subscribeToPartitionChanges(PartitionChangeCallbackFn fn);

  void publishPartitionChange(RefPtr<PartitionChangeNotification> change);

protected:

  Option<RefPtr<Table>> findTableWithLock(
      const String& tsdb_namespace,
      const String& table_name) const;

  void loadPartitions(const Vector<PartitionKey>& partitions);

  String db_path_;
  RefPtr<mdb::MDB> db_;

  mutable std::mutex mutex_;
  HashMap<String, RefPtr<Table>> tables_;
  HashMap<String, ScopedPtr<LazyPartition>> partitions_;
  Vector<PartitionChangeCallbackFn> callbacks_;
};

} // namespace tdsb

