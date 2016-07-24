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
#include <eventql/util/stdtypes.h>
#include <eventql/util/random.h>
#include <eventql/util/option.h>
#include <eventql/util/mdb/MDB.h>
#include <eventql/db/table_config.pb.h>
#include <eventql/db/partition.h>
#include <eventql/db/partition.h>
#include <eventql/db/table_info.h>
#include <eventql/db/partition_info.pb.h>
#include <eventql/db/partition_change_notification.h>
#include <eventql/db/record_envelope.pb.h>
#include <eventql/db/server_config.h>
#include <eventql/server/server_stats.h>

#include "eventql/eventql.h"

namespace eventql {

class PartitionMap {
public:

  PartitionMap(ServerCfg* cfg);

  void configureTable(
      const TableDefinition& config,
      Set<SHA1Hash>* affected_partitions = nullptr);

  void open();

  Option<RefPtr<Table>> findTable(
      const String& tsdb_namespace,
      const String& table_name) const;

  void listTables(
      const String& tsdb_namespace,
      Function<void (const TSDBTableInfo& table)> fn) const;

  void listTablesReverse(
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

  /**
   * Attempt to drop a partition from the local host (i.e. delete the partition
   * data).
   *
   * The partition will only be deleted if:
   *
   *   - the partition was replicated to and acknowledged by at least N other
   *     hosts. I.e. we can be sure at least N other copies of the data exist
   *     in the cluster. N is the min. number of copies as configured by the 
   *     replication schema.
   *
   *   - the local host does not appear in the preference list for this
   *     partition, i.e. the local host should not store this partition (this
   *     condition is only met after a cluster rebalance or after a failover
   *     store into a non-owning replica)
   *
   * Note that this method has an inherent race condition: A caller would
   * usually check that the partition is fully replicated and not locally owned
   * before calling this method. However there is no way to ensure that no
   * intermittent stores happen in the partition (in between checking the
   * precondition and calling this method). So the method returns a boolean
   * indicating success or failure. 
   *
   * The method will return true iff the partition was succesfully dropped. A
   * false return value indicates that one of the two conditions above weren't
   * met.
   */
  bool dropLocalPartition(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key);

protected:

  Option<RefPtr<Table>> findTableWithLock(
      const String& tsdb_namespace,
      const String& table_name) const;

  void loadPartitions(const Vector<PartitionKey>& partitions);

  ServerCfg* cfg_;
  RefPtr<mdb::MDB> db_;

  mutable std::mutex mutex_;
  ConfigDirectory* cdir_;
  OrderedMap<String, RefPtr<Table>> tables_;
  OrderedMap<String, ScopedPtr<LazyPartition>> partitions_;
  Vector<PartitionChangeCallbackFn> callbacks_;
};

} // namespace tdsb

