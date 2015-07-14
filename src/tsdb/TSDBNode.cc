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

TSDBNode::TSDBNode(
    const String& db_path,
    RefPtr<dproc::ReplicationScheme> replication_scheme,
    http::HTTPConnectionPool* http) :
    noderef_{
        .db_path = db_path,
        .db = mdb::MDB::open(
            db_path,
            false,
            1024 * 1024 * 1024, // 1 GiB
            "index.db",
            "index.db.lck"),
        .replication_scheme = replication_scheme,
        .http = http},
    sql_engine_(this) {}

// FIXPAUL proper longest prefix search ;)
TableConfig* TSDBNode::configFor(
    const String& stream_ns,
    const String& stream_key) const {
  TableConfig* config = nullptr;
  size_t match_len = 0;

  auto stream_ns_key = stream_ns + "~" + stream_key;
  for (const auto& cfg : configs_) {
    if (!StringUtil::beginsWith(stream_ns_key, cfg.first)) {
      continue;
    }

    if (cfg.first.length() > match_len) {
      config = cfg.second.get();
      match_len = cfg.first.length();
    }
  }

  if (config == nullptr) {
    RAISEF(kIndexError, "no config found for stream key: '$0'", stream_key);
  }

  return config;
}

void TSDBNode::configure(const TSDBNodeConfig& conf, const String& base_path) {
  for (const auto& schema_file : conf.include_protobuf_schema()) {
    schemas_.loadProtobufFile(base_path, schema_file);
  }

  for (const auto& sc : conf.event_stream()) {
    auto stream_ns_key = sc.tsdb_namespace() + "~" + sc.table_name();

    configs_.emplace_back(
        stream_ns_key,
        ScopedPtr<TableConfig>(new TableConfig(sc)));
  }
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

    auto config = configFor(tsdb_namespace, state.stream_key);
    auto partition = Partition::reopen(
        partition_key,
        state,
        db_key,
        schemas_.getSchema(config->schema()),
        config,
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

  auto config = configFor(tsdb_namespace, stream_key);
  auto partition = Partition::create(
      partition_key,
      stream_key,
      db_key,
      schemas_.getSchema(config->schema()),
      config,
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
    Function<void (const TSDBTableInfo& table)> fn) const {
  for (const auto& tbl : configs_) {
    TSDBTableInfo ti;
    ti.table_name = tbl.first;
    ti.config = *tbl.second;
    ti.schema = schemas_.getSchema(ti.config.schema());
    fn(ti);
  }
}

Option<TSDBTableInfo> TSDBNode::tableInfo(
      const String& tsdb_namespace,
      const String& table_key) const {

  auto config = configFor(tsdb_namespace, table_key);
  if (config == nullptr) {
    return None<TSDBTableInfo>();
  }

  TSDBTableInfo ti;
  ti.table_name = table_key;
  ti.config = *config;
  ti.schema = schemas_.getSchema(config->schema());
  return Some(ti);
}

const String& TSDBNode::dbPath() const {
  return noderef_.db_path;
}

SQLEngine* TSDBNode::sqlEngine() {
  return &sql_engine_;
}

} // namespace tdsb

