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
#include <thread>
#include <eventql/util/util/Base64.h>
#include <eventql/util/fnv.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/io/sstable/sstablereader.h>
#include <eventql/core/PartitionMap.h>
#include <eventql/core/PartitionState.pb.h>
#include <eventql/core/PartitionReplication.h>

#include "eventql/eventql.h"

namespace eventql {

static mdb::MDBOptions tsdb_mdb_opts() {
  mdb::MDBOptions opts;
  opts.data_filename = "index.db";
  opts.lock_filename = "index.db.lck";
  return opts;
};

PartitionMap::PartitionMap(
    ServerConfig* cfg) :
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

void PartitionMap::configureTable(const TableDefinition& table) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto tbl_key = table.customer() + "~" + table.table_name();

  auto iter = tables_.find(tbl_key);
  if (iter == tables_.end()) {
    tables_.emplace(tbl_key, new Table(table));
  } else {
    iter->second->updateConfig(table);
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

    partitions_.emplace(db_key, mkScoped(new LazyPartition()));

    if (table.isEmpty()) {
      logWarning(
          "tsdb",
          "Orphaned partition: $0/$1",
          table_key,
          partition_key.toString());
      continue;
    }

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

  std::unique_lock<std::mutex> lk(mutex_);
  auto iter = partitions_.find(db_key);
  if (iter != partitions_.end()) {
    if (iter->second->isLoaded()) {
      return iter->second->getPartition();
    }
  }

  auto table = findTableWithLock(tsdb_namespace, table_name);
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

  auto partition = Partition::create(
      tsdb_namespace,
      table.get(),
      partition_key,
      cfg_);

  partitions_.emplace(db_key, mkScoped(new LazyPartition(partition)));

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

  std::unique_lock<std::mutex> lk(mutex_);
  auto iter = partitions_.find(db_key);
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

void PartitionMap::listTables(
    const String& tsdb_namespace,
    Function<void (const TSDBTableInfo& table)> fn) const {
  for (const auto& tbl : tables_) {
    if (tbl.second->tsdbNamespace() != tsdb_namespace) {
      continue;
    }

    TSDBTableInfo ti;
    ti.table_name = tbl.second->name();
    ti.config = tbl.second->config();
    ti.schema = tbl.second->schema();
    fn(ti);
  }
}

void PartitionMap::listTablesReverse(
    const String& tsdb_namespace,
    Function<void (const TSDBTableInfo& table)> fn) const {
  for (auto cur = tables_.rbegin(); cur != tables_.rend(); ++cur) {
    if (cur->second->tsdbNamespace() != tsdb_namespace) {
      continue;
    }

    TSDBTableInfo ti;
    ti.table_name = cur->second->name();
    ti.config = cur->second->config();
    ti.schema = cur->second->schema();
    fn(ti);
  }
}

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
  size_t full_copies = 0;
  try {
    auto repl_scheme = cfg_->repl_scheme;

    full_copies = partition
        ->getReplicationStrategy(repl_scheme, nullptr)
        ->numFullRemoteCopies();

    if (repl_scheme->hasLocalReplica(partition_key) ||
        full_copies < repl_scheme->minNumCopies()) {
      RAISE(kIllegalStateError, "can't delete partition");
    }
  } catch (const StandardException& e) {
    /* unlock partition and bail */
    partition_writer->unlock();
    return false;
  }

  /* start deletion */
  logInfo(
      "z1.core",
      "Partition $0/$1/$2 is not owned by this node and has $3 other " \
      "full copies, trying to unload and drop",
      tsdb_namespace,
      table_name,
      partition_key.toString(),
      full_copies);

  /* freeze partition and unlock waiting writers (they will fail) */
  partition_writer->freeze();
  partition_writer->unlock();

  auto db_key = tsdb_namespace + "~";
  db_key.append((char*) partition_key.data(), partition_key.size());

  /* grab the main lock */
  std::unique_lock<std::mutex> lk(mutex_);

  /* delete from in memory partition map */
  auto iter = partitions_.find(db_key);
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

  /* delete partition data from disk (move to trash) */
  {
    auto src_path = FileUtil::joinPaths(
        cfg_->db_path,
        StringUtil::format(
            "$0/$1/$2",
            tsdb_namespace,
            SHA1::compute(table_name).toString(),
            partition_key.toString()));

    auto dst_path = FileUtil::joinPaths(
        cfg_->db_path,
        StringUtil::format(
            "../../trash/$0~$1~$2~$3",
            tsdb_namespace,
            SHA1::compute(table_name).toString(),
            partition_key.toString(),
            Random::singleton()->hex64()));

    FileUtil::mv(src_path, dst_path);
  }

  return true;
}

Option<TSDBTableInfo> PartitionMap::tableInfo(
      const String& tsdb_namespace,
      const String& table_key) const {
  auto table = findTable(tsdb_namespace, table_key);
  if (table.isEmpty()) {
    return None<TSDBTableInfo>();
  }

  TSDBTableInfo ti;
  ti.table_name = table.get()->name();
  ti.config = table.get()->config();
  ti.schema = table.get()->schema();
  return Some(ti);
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

