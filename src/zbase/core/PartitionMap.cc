/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <thread>
#include <stx/util/Base64.h>
#include <stx/fnv.h>
#include <stx/protobuf/msg.h>
#include <stx/io/fileutil.h>
#include <sstable/sstablereader.h>
#include <zbase/core/PartitionMap.h>
#include <zbase/core/PartitionState.pb.h>

using namespace stx;

namespace tsdb {

static mdb::MDBOptions tsdb_mdb_opts() {
  mdb::MDBOptions opts;
  opts.data_filename = "index.db";
  opts.lock_filename = "index.db.lck";
  return opts;
};

PartitionMap::PartitionMap(const String& db_path) :
    db_path_(db_path),
    db_(mdb::MDB::open(db_path, tsdb_mdb_opts())) {}

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
  auto tbl_key = table.tsdb_namespace() + "~" + table.table_name();

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
        db_path_,
        this);
  }

  auto partition = Partition::create(
      tsdb_namespace,
      table.get(),
      partition_key,
      db_path_);

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
            db_path_,
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

