/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord/util/Base64.h>
#include <tsdb/TSDBNode.h>

using namespace fnord;

namespace tsdb {

static mdb::MDBOptions tsdb_mdb_opts() {
  mdb::MDBOptions opts;
  opts.data_filename = "index.db";
  opts.lock_filename = "index.db.lck";
  return opts;
};

TSDBNode::TSDBNode(
    const String& db_path,
    RefPtr<dproc::ReplicationScheme> replication_scheme,
    http::HTTPConnectionPool* http) :
    noderef_(
        db_path,
        mdb::MDB::open(
            db_path,
            tsdb_mdb_opts()),
        replication_scheme,
        http) {}

Option<RefPtr<Table>> TSDBNode::findTable(
    const String& stream_ns,
    const String& stream_key) const {
  std::unique_lock<std::mutex> lk(mutex_);
  auto stream_ns_key = stream_ns + "~" + stream_key;

  const auto& iter = tables_.find(stream_ns_key);
  if (iter == tables_.end()) {
    return None<RefPtr<Table>>();
  } else {
    return Some(iter->second);
  }
}

void TSDBNode::configure(const TSDBNodeConfig& conf, const String& base_path) {
  for (const auto& schema_file : conf.include_protobuf_schema()) {
    schemas_.loadProtobufFile(base_path, schema_file);
  }

  for (const auto& sc : conf.event_stream()) {
    createTable(sc);
  }
}

void TSDBNode::createTable(const TableConfig& table) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto stream_ns_key = table.tsdb_namespace() + "~" + table.table_name();

  tables_.emplace(
      stream_ns_key,
      new Table(
          table,
          schemas_.getSchema(table.schema())));
}

void TSDBNode::start(
    size_t num_compaction_threads,
    size_t num_replication_threads) {
  reopenPartitions();

  for (int i = 0; i < num_compaction_threads; ++i) {
    compaction_workers_.emplace_back(new CompactionWorker(&noderef_));
    compaction_workers_.back()->start();
  }

  for (int i = 0; i < num_replication_threads; ++i) {
    replication_workers_.emplace_back(new ReplicationWorker(&noderef_));
    replication_workers_.back()->start();
  }
}

void TSDBNode::stop() {
  for (auto& w : compaction_workers_) {
    w->stop();
  }

  for (auto& w : replication_workers_) {
    w->stop();
  }
}

void TSDBNode::reopenPartitions() {
  auto txn = noderef_.db->startTransaction(false);
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

    util::BinaryMessageReader reader(value.data(), value.size());
    PartitionState state;
    state.decode(&reader);

    auto partition = Partition::reopen(
        partition_key,
        state,
        db_key,
        findTable(tsdb_namespace, state.stream_key).get(),
        &noderef_);

    partitions_.emplace(db_key, partition);
  }

  cursor->close();
  txn->abort();
}

RefPtr<Partition> TSDBNode::findOrCreatePartition(
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

  auto partition = Partition::create(
      partition_key,
      stream_key,
      db_key,
      findTable(tsdb_namespace, stream_key).get(),
      &noderef_);

  partitions_.emplace(db_key, partition);
  return partition;
}

Option<RefPtr<Partition>> TSDBNode::findPartition(
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

void TSDBNode::listTables(
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

Option<TSDBTableInfo> TSDBNode::tableInfo(
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

const String& TSDBNode::dbPath() const {
  return noderef_.db_path;
}

} // namespace tdsb

