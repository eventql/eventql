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
#include <thread>
#include <eventql/util/util/Base64.h>
#include <eventql/util/fnv.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/io/sstable/sstablereader.h>
#include <eventql/db/partition_map.h>
#include <eventql/db/partition_state.pb.h>
#include <eventql/db/partition_replication.h>
#include <eventql/db/metadata_coordinator.h>
#include <eventql/db/partition_reader.h>
#include <eventql/db/partition_writer.h>
#include "eventql/eventql.h"
#include "eventql/db/file_tracker.h"

namespace eventql {

static mdb::MDBOptions tsdb_mdb_opts() {
  mdb::MDBOptions opts;
  opts.data_filename = "index.db";
  opts.lock_filename = "index.db.lck";
  return opts;
};

PartitionMap::PartitionMap(
    DatabaseContext* cfg) :
    cfg_(cfg),
    db_(mdb::MDB::open(cfg_->db_path, tsdb_mdb_opts())) {}

Option<RefPtr<Table>> PartitionMap::findTable(
    const String& stream_ns,
    const String& table_name) const {
  std::unique_lock<std::mutex> lk(mutex_);
  return findTableWithLock(stream_ns, table_name);
}

Option<RefPtr<Table>> PartitionMap::findTableWithLock(
    const String& stream_ns,
    const String& table_name) const {
  auto stream_ns_key = stream_ns + "~" + table_name;

  const auto& iter = tables_.find(stream_ns_key);
  if (iter == tables_.end()) {
    return None<RefPtr<Table>>();
  } else {
    return Some(iter->second);
  }
}

void PartitionMap::configureTable(
    const TableDefinition& table,
    Set<SHA1Hash>* affected_partitions /* = nullptr */) {
  if (table.config().partitioner() == TBL_PARTITION_FIXED) {
    return;
  }

  if (table.config().storage() != TBL_STORAGE_COLSM) {
    return;
  }

  std::unique_lock<std::mutex> lk(mutex_);
  auto tbl_key = table.customer() + "~" + table.table_name();
  bool metadata_changed = false;

  {
    auto iter = tables_.find(tbl_key);
    if (iter == tables_.end()) {
      tables_.emplace(tbl_key, new Table(table));
    } else {
      if (table.config().partitioner() != TBL_PARTITION_FIXED) {
        auto last_metadata_txn = iter->second->getLastMetadataTransaction();
        if (table.metadata_txnseq() > last_metadata_txn.getSequenceNumber()) {
          metadata_changed = true;
        }
      }

      iter->second->updateConfig(table);
    }
  }

  if (metadata_changed && affected_partitions != nullptr) {
    auto key_prefix = tbl_key + "~";
    auto iter = partitions_.lower_bound(key_prefix);
    for (; iter != partitions_.end(); ++iter) {
      if (!StringUtil::beginsWith(iter->first, key_prefix)) {
        break;
      }

      auto partition_id_str = iter->first.substr(key_prefix.size());
      affected_partitions->emplace(
          SHA1Hash(partition_id_str.data(), partition_id_str.size()));
    }
  }
}

void PartitionMap::open() {
  auto txn = db_->startTransaction(false);
  auto cursor = txn->getCursor();

  Vector<PartitionKey> partitions;
  for (int i = 0; ; ++i) {
    Buffer key;
    Buffer value;
    if (i == 0) {
      if (!cursor->getFirst(&key, &value)) {
        break;
      }
    } else {
      if (!cursor->getNext(&key, &value)) {
        break;
      }
    }

    auto db_key = key.toString();
    if (db_key.size() == 0) {
      continue;
    }

    if (db_key[0] == 0x1b) {
      continue;
    }

    auto tsdb_namespace_off = StringUtil::find(db_key, '~');
    if (tsdb_namespace_off == String::npos) {
      RAISEF(kRuntimeError, "invalid partition key: $0", db_key);
    }

    auto tsdb_namespace = db_key.substr(0, tsdb_namespace_off);
    SHA1Hash partition_key(
        db_key.data() + tsdb_namespace_off + 1,
        db_key.size() - tsdb_namespace_off - 1);

    auto table_key = value.toString();
    auto table = findTableWithLock(tsdb_namespace, table_key);

    if (table.isEmpty()) {
      logWarning(
          "tsdb",
          "Orphaned partition: $0/$1",
          table_key,
          partition_key.toString());
      continue;
    }

    auto mem_key = tsdb_namespace + "~" + table_key + "~";
    mem_key.append((char*) partition_key.data(), partition_key.size());

    partitions_.emplace(mem_key, mkScoped(new LazyPartition()));

    partitions.emplace_back(
        std::make_tuple(tsdb_namespace, table_key, partition_key));
  }

  cursor->close();
  txn->abort();

  auto background_load_thread = std::thread(
      std::bind(&PartitionMap::loadPartitions, this, partitions));

  background_load_thread.detach();
}

void PartitionMap::loadPartitions(const Vector<PartitionKey>& partitions) {
  for (const auto& p : partitions) {
    try {
      findPartition(std::get<0>(p), std::get<1>(p), std::get<2>(p));
    } catch (const StandardException& e) {
      logError(
          "tsdb",
          e,
          "error while loading partition $0/$1/$2",
          std::get<0>(p),
          std::get<1>(p),
          std::get<2>(p).toString());
    }
  }
}

RefPtr<Partition> PartitionMap::findOrCreatePartition(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key) {
  auto db_key = tsdb_namespace + "~";
  db_key.append((char*) partition_key.data(), partition_key.size());

  auto mem_key = tsdb_namespace + "~" + table_name + "~";
  mem_key.append((char*) partition_key.data(), partition_key.size());

  std::unique_lock<std::mutex> lk(mutex_);
  Option<RefPtr<Table>> table;
  {
    auto iter = partitions_.find(mem_key);
    if (iter != partitions_.end()) {
      if (iter->second->isLoaded()) {
        return iter->second->getPartition();
      }
    }

    table = findTableWithLock(tsdb_namespace, table_name);
    if (table.isEmpty()) {
      RAISEF(kNotFoundError, "table not found: $0", table_name);
    }

    if (iter != partitions_.end()) {
      auto partition = iter->second.get();
      lk.unlock();

      return partition->getPartition(
          tsdb_namespace,
          table.get(),
          partition_key,
          cfg_,
          this);
    }
  }

  lk.unlock();

  PartitionDiscoveryResponse discovery_info;

  if (table.get()->partitionerType() != TBL_PARTITION_FIXED) {
    PartitionDiscoveryRequest discovery_request;
    discovery_request.set_db_namespace(tsdb_namespace);
    discovery_request.set_table_id(table_name);
    discovery_request.set_min_txnseq(0);
    discovery_request.set_partition_id(
        partition_key.data(),
        partition_key.size());
    discovery_request.set_lookup_by_id(true);

    MetadataCoordinator coordinator(cfg_->config_directory);
    PartitionDiscoveryResponse discovery_response;
    auto rc = coordinator.discoverPartition(
        discovery_request,
        &discovery_info);

    if (!rc.isSuccess()) {
      RAISE(kRuntimeError, rc.message());
    }

    switch (discovery_info.code()) {
      case PDISCOVERY_LOAD:
      case PDISCOVERY_SERVE:
        break;
      default:
        RAISE(kRuntimeError, "trying to load unassigned partition");
    }
  }

  lk.lock();
  {
    auto iter = partitions_.find(mem_key);
    if (iter != partitions_.end()) {
      return iter->second->getPartition(
          tsdb_namespace,
          table.get(),
          partition_key,
          cfg_,
          this);
    }
  }

  auto partition = Partition::create(
      tsdb_namespace,
      table.get(),
      partition_key,
      discovery_info,
      cfg_);

  partitions_.emplace(mem_key, mkScoped(new LazyPartition(partition)));

  auto txn = db_->startTransaction(false);
  txn->update(
      db_key.data(),
      db_key.size(),
      table_name.data(),
      table_name.size());

  txn->commit();
  lk.unlock();

  auto change = mkRef(new PartitionChangeNotification());
  change->partition = partition;
  publishPartitionChange(change);

  return partition;
}

Option<RefPtr<Partition>> PartitionMap::findPartition(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key) {
  auto db_key = tsdb_namespace + "~";
  db_key.append((char*) partition_key.data(), partition_key.size());

  auto mem_key = tsdb_namespace + "~" + table_name + "~";
  mem_key.append((char*) partition_key.data(), partition_key.size());

  std::unique_lock<std::mutex> lk(mutex_);
  auto iter = partitions_.find(mem_key);
  if (iter == partitions_.end()) {
    return None<RefPtr<Partition>>();
  } else {
    if (iter->second->isLoaded()) {
      return Some(iter->second->getPartition());
    }

    auto table = findTableWithLock(tsdb_namespace, table_name);
    if (table.isEmpty()) {
      RAISEF(kNotFoundError, "table not found: $0", table_name);
    }

    auto partition = iter->second.get();
    lk.unlock();

    return Some(
        partition->getPartition(
            tsdb_namespace,
            table.get(),
            partition_key,
            cfg_,
            this));
  }
}

//void PartitionMap::listTables(
//    const String& tsdb_namespace,
//    Function<void (const TSDBTableInfo& table)> fn) const {
//  for (const auto& tbl : tables_) {
//    if (tbl.second->tsdbNamespace() != tsdb_namespace) {
//      continue;
//    }
//
//    TSDBTableInfo ti;
//    ti.table_name = tbl.second->name();
//    ti.config = tbl.second->config();
//    ti.schema = tbl.second->schema();
//    fn(ti);
//  }
//}
//
//void PartitionMap::listTablesReverse(
//    const String& tsdb_namespace,
//    Function<void (const TSDBTableInfo& table)> fn) const {
//  for (auto cur = tables_.rbegin(); cur != tables_.rend(); ++cur) {
//    if (cur->second->tsdbNamespace() != tsdb_namespace) {
//      continue;
//    }
//
//    TSDBTableInfo ti;
//    ti.table_name = cur->second->name();
//    ti.config = cur->second->config();
//    ti.schema = cur->second->schema();
//    fn(ti);
//  }
//}

bool PartitionMap::dropLocalPartition(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key) {
  auto partition_opt = findPartition(tsdb_namespace, table_name, partition_key);
  if (partition_opt.isEmpty()) {
    RAISE(kNotFoundError, "partition not found");
  }

  auto partition = partition_opt.get();
  auto partition_writer = partition->getWriter();

  /* lock partition */
  partition_writer->lock();

  /* check preconditions */
  try {
    auto repl = partition->getReplicationStrategy();
    if (!repl->shouldDropPartition()) {
      RAISE(kIllegalStateError, "can't delete partition");
    }
  } catch (const StandardException& e) {
    /* unlock partition and bail */
    partition_writer->unlock();
    return false;
  }

  /* start deletion */
  logInfo(
      "evqld",
      "Partition $0/$1/$2 is not owned by this node and is fully replicated," \
      " trying to unload and drop",
      tsdb_namespace,
      table_name,
      partition_key.toString());

  /* freeze partition and unlock waiting writers (they will fail) */
  partition_writer->freeze();
  partition_writer->unlock();

  auto db_key = tsdb_namespace + "~";
  db_key.append((char*) partition_key.data(), partition_key.size());

  auto mem_key = tsdb_namespace + "~" + table_name + "~";
  mem_key.append((char*) partition_key.data(), partition_key.size());

  /* grab the main lock */
  std::unique_lock<std::mutex> lk(mutex_);

  /* delete from in memory partition map */
  auto iter = partitions_.find(mem_key);
  if (iter == partitions_.end()) {
    /* somebody else already deleted this partition */
    return true;
  } else {
    partitions_.erase(iter);
  }

  /* delete from on disk partition map */
  auto txn = db_->startTransaction(false);
  txn->del(db_key);
  txn->commit();

  /* move partition data to trash */
  cfg_->file_tracker->deleteFile(
      StringUtil::format(
          "$0/$1/$2",
          tsdb_namespace,
          SHA1::compute(table_name).toString(),
          partition_key.toString()));

  return true;
}

void PartitionMap::dropPartition(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key) {
  auto partition_opt = findPartition(tsdb_namespace, table_name, partition_key);
  if (partition_opt.isEmpty()) {
    RAISE(kNotFoundError, "partition not found");
  }

  auto table_config = cfg_->config_directory->getTableConfig(
      tsdb_namespace,
      table_name);

  auto partition = partition_opt.get();
  auto partition_writer = partition->getWriter();

  /* lock partition */
  partition_writer->lock();

  /* check preconditions */
  auto partition_gen = partition->getSnapshot()->state.table_generation();
  if (partition_gen >= table_config.generation()) {
    partition_writer->unlock();
    return;
  }

  /* start deletion */
  logInfo(
      "evqld",
      "Dropping partition $0/$1/$2",
      tsdb_namespace,
      table_name,
      partition_key.toString());

  /* freeze partition and unlock waiting writers (they will fail) */
  partition_writer->freeze();
  partition_writer->unlock();

  /* prepare access keys */
  auto db_key = tsdb_namespace + "~";
  db_key.append((char*) partition_key.data(), partition_key.size());

  auto mem_key = tsdb_namespace + "~" + table_name + "~";
  mem_key.append((char*) partition_key.data(), partition_key.size());

  /* grab the main lock */
  std::unique_lock<std::mutex> lk(mutex_);

  /* delete from in memory partition map */
  auto iter = partitions_.find(mem_key);
  if (iter == partitions_.end()) {
    /* somebody else already deleted this partition */
    return;
  } else {
    partitions_.erase(iter);
  }

  /* delete from on disk partition map */
  auto txn = db_->startTransaction(false);
  txn->del(db_key);
  txn->commit();

  return;
}

void PartitionMap::subscribeToPartitionChanges(PartitionChangeCallbackFn fn) {
  callbacks_.emplace_back(fn);
}

void PartitionMap::publishPartitionChange(
    RefPtr<PartitionChangeNotification> change) {
  for (const auto& cb : callbacks_) {
    cb(change);
  }
}


} // namespace tdsb

