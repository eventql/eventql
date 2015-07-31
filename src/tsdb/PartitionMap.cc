/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/util/Base64.h>
#include <stx/fnv.h>
#include <stx/protobuf/msg.h>
#include <stx/io/fileutil.h>
#include <sstable/sstablereader.h>
#include <tsdb/PartitionMap.h>
#include <tsdb/PartitionState.pb.h>

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
    const String& stream_key) const {
  std::unique_lock<std::mutex> lk(mutex_);
  return findTableWithLock(stream_ns, stream_key);
}

Option<RefPtr<Table>> PartitionMap::findTableWithLock(
    const String& stream_ns,
    const String& stream_key) const {
  auto stream_ns_key = stream_ns + "~" + stream_key;

  const auto& iter = tables_.find(stream_ns_key);
  if (iter == tables_.end()) {
    return None<RefPtr<Table>>();
  } else {
    return Some(iter->second);
  }
}

void PartitionMap::configureTable(const TableConfig& table) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto stream_ns_key = table.tsdb_namespace() + "~" + table.table_name();

  tables_.emplace(
      stream_ns_key,
      new Table(
          table,
          msg::MessageSchema::decode(table.schema())));
}

void PartitionMap::open() {
  auto txn = db_->startTransaction(false);
  auto cursor = txn->getCursor();

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

    auto partition = Partition::reopen(
        tsdb_namespace,
        table.get(),
        partition_key,
        db_path_);

    partitions_.emplace(db_key, partition);
  }

  cursor->close();
  txn->abort();
}

RefPtr<Partition> PartitionMap::findOrCreatePartition(
    const String& tsdb_namespace,
    const String& stream_key,
    const SHA1Hash& partition_key) {
  auto db_key = tsdb_namespace + "~";
  db_key.append((char*) partition_key.data(), partition_key.size());

  std::unique_lock<std::mutex> lk(mutex_);
  auto iter = partitions_.find(db_key);
  if (iter != partitions_.end()) {
    return iter->second;
  }

  auto table = findTableWithLock(tsdb_namespace, stream_key);
  if (table.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", stream_key);
  }

  auto partition = Partition::create(
      tsdb_namespace,
      table.get(),
      partition_key,
      db_path_);

  partitions_.emplace(db_key, partition);

  auto txn = db_->startTransaction(false);
  txn->update(
      db_key.data(),
      db_key.size(),
      stream_key.data(),
      stream_key.size());

  txn->commit();

  return partition;
}

Option<RefPtr<Partition>> PartitionMap::findPartition(
    const String& tsdb_namespace,
    const String& stream_key,
    const SHA1Hash& partition_key) {
  auto db_key = tsdb_namespace + "~";
  db_key.append((char*) partition_key.data(), partition_key.size());

  std::unique_lock<std::mutex> lk(mutex_);
  auto iter = partitions_.find(db_key);
  if (iter == partitions_.end()) {
    return None<RefPtr<Partition>>();
  } else {
    return Some(iter->second);
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

