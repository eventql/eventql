/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-tsdb/TSDBNode.h>

namespace fnord {
namespace tsdb {

TSDBNode::TSDBNode(
    const String& nodeid,
    const String& db_path) :
    nodeid_(nodeid),
    noderef_{
        .db_path = db_path,
        .db = mdb::MDB::open(
            db_path,
            false,
            1024 * 1024 * 1024, // 1 GiB
            nodeid + ".db",
            nodeid + ".db.lck")} {}

void TSDBNode::insertRecord(
    const String& stream_key,
    uint64_t record_id,
    const Buffer& record,
    DateTime time) {
  auto config = configFor(stream_key);
  auto chunk_key = StreamChunk::streamChunkKeyFor(stream_key, time, *config);

  RefPtr<StreamChunk> chunk(nullptr);
  {
    std::unique_lock<std::mutex> lk(mutex_);
    auto chunk_iter = chunks_.find(chunk_key);
    if (chunk_iter == chunks_.end()) {
      chunk = StreamChunk::create(chunk_key, stream_key, config, &noderef_);
      chunks_.emplace(chunk_key, chunk);
    } else {
      chunk = chunk_iter->second;
    }
  }

  chunk->insertRecord(record_id, record, time);
}

// FIXPAUL proper longest prefix search ;)
RefPtr<StreamProperties> TSDBNode::configFor(const String& stream_key) const {
  RefPtr<StreamProperties> config(nullptr);
  size_t match_len = 0;

  for (const auto& cfg : configs_) {
    if (!StringUtil::beginsWith(stream_key, cfg.first)) {
      continue;
    }

    if (cfg.first.length() > match_len) {
      config = cfg.second;
      match_len = cfg.first.length();
    }
  }

  if (config.get() == nullptr) {
    RAISEF(kIndexError, "no config found for stream key: '$0'", stream_key);
  }

  return config;
}

void TSDBNode::configurePrefix(
    const String& stream_key_prefix,
    StreamProperties props) {
  configs_.emplace_back(stream_key_prefix, new StreamProperties(props));
}

void TSDBNode::start() {
  reopenStreamChunks();
  compaction_workers_.emplace_back(new CompactionWorker(&noderef_));
  compaction_workers_.back()->start();
}

void TSDBNode::stop() {
  for (auto& w : compaction_workers_) {
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

    auto chunk_key = key.toString();
    util::BinaryMessageReader reader(value.data(), value.size());
    StreamChunkState state;
    state.decode(&reader);

    auto chunk = StreamChunk::reopen(
        chunk_key,
        state,
        configFor(state.stream_key),
        &noderef_);

    chunks_.emplace(chunk_key, chunk);
  }

  cursor->close();
  txn->abort();
}

} // namespace tdsb
} // namespace fnord

