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
    RefPtr<dht::ReplicationScheme> replication_scheme,
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
        .http = http} {}

// FIXPAUL proper longest prefix search ;)
StreamConfig* TSDBNode::configFor(
    const String& stream_ns,
    const String& stream_key) const {
  StreamConfig* config = nullptr;
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

void TSDBNode::configurePrefix(
    const String& stream_ns,
    StreamConfig config) {
  auto stream_ns_key = stream_ns + "~" + config.stream_key_prefix();

  configs_.emplace_back(
      stream_ns_key,
      ScopedPtr<StreamConfig>(new StreamConfig(config)));
}

void TSDBNode::start(
    size_t num_compaction_threads,
    size_t num_replication_threads) {
  reopenStreamChunks();

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

void TSDBNode::reopenStreamChunks() {
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

    auto partition_ns_key = key.toString();
    if (partition_ns_key.size() == 0) {
      continue;
    }

    if (partition_ns_key[0] == 0x1b) {
      continue;
    }

    auto partition_ns_off = StringUtil::find(partition_ns_key, '~');
    if (partition_ns_off == String::npos) {
      RAISEF(kRuntimeError, "invalid partition key: $0", partition_ns_key);
    }

    auto partition_ns = partition_ns_key.substr(0, partition_ns_off);
    SHA1Hash partition_key(
        partition_ns_key.data() + partition_ns_off + 1,
        partition_ns_key.size() - partition_ns_off - 1);

    util::BinaryMessageReader reader(value.data(), value.size());
    StreamChunkState state;
    state.decode(&reader);

    auto chunk = StreamChunk::reopen(
        partition_key,
        state,
        configFor(partition_ns, state.stream_key),
        &noderef_);

    chunks_.emplace(partition_ns_key, chunk);
  }

  cursor->close();
  txn->abort();
}

} // namespace tdsb

