/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/Partition.h>
#include <fnord/io/fileutil.h>
#include <fnord/uri.h>
#include <fnord/util/binarymessagewriter.h>
#include <fnord/wallclock.h>
#include <fnord/protobuf/MessageEncoder.h>
#include <fnord/protobuf/msg.h>

using namespace fnord;

namespace tsdb {

RefPtr<Partition> Partition::create(
    const SHA1Hash& partition_key,
    const String& stream_key,
    const String& db_key,
    StreamConfig* config,
    TSDBNodeRef* node) {

  fnord::logDebug(
      "tsdb",
      "Creating new partition; stream='$0' partition='$1'",
      stream_key,
      partition_key.toString());

  return RefPtr<Partition>(
      new Partition(
          partition_key,
          stream_key,
          db_key,
          config,
          node));
}

RefPtr<Partition> Partition::reopen(
    const SHA1Hash& partition_key,
    const PartitionState& state,
    const String& db_key,
    StreamConfig* config,
    TSDBNodeRef* node) {

  fnord::logDebug(
      "tsdb",
      "Loading partition; stream='$0' partition='$1'",
      state.stream_key,
      partition_key.toString());

  return RefPtr<Partition>(
      new Partition(
          partition_key,
          state,
          db_key,
          config,
          node));
}

Partition::Partition(
    const SHA1Hash& partition_key,
    const String& stream_key,
    const String& db_key,
    StreamConfig* config,
    TSDBNodeRef* node) :
    stream_key_(stream_key),
    key_(partition_key),
    db_key_(db_key),
    config_(config),
    node_(node),
    records_(
        node->db_path,
        key_.toString().substr(0, 12)  + "."),
    last_compaction_(0) {
  records_.setMaxDatafileSize(config_->max_sstable_size());
}

Partition::Partition(
    const SHA1Hash& partition_key,
    const PartitionState& state,
    const String& db_key,
    StreamConfig* config,
    TSDBNodeRef* node) :
    stream_key_(state.stream_key),
    key_(partition_key),
    db_key_(db_key),
    config_(config),
    node_(node),
    records_(
        node->db_path,
        key_.toString().substr(0, 12) + ".",
        state.record_state),
    repl_offsets_(state.repl_offsets),
    last_compaction_(0) {
  scheduleCompaction();
  node_->compactionq.insert(this, WallClock::unixMicros());
  node_->replicationq.insert(this, WallClock::unixMicros());
  records_.setMaxDatafileSize(config_->max_sstable_size());
}

void Partition::insertRecord(
    const SHA1Hash& record_id,
    const Buffer& record) {
  std::unique_lock<std::mutex> lk(mutex_);

  fnord::logTrace(
      "tsdb",
      "Insert 1 record into stream='$0' partition='$1'",
      stream_key_,
      key_.toString());

  auto old_ver = records_.version();
  records_.addRecord(record_id, record);
  if (records_.version() != old_ver) {
    commitState();
  }

  scheduleCompaction();
}

void Partition::insertRecords(const Vector<RecordRef>& records) {
  std::unique_lock<std::mutex> lk(mutex_);

  fnord::logTrace(
      "tsdb",
      "Insert $0 records into stream='$1'",
      records.size(),
      stream_key_);

  auto old_ver = records_.version();
  records_.addRecords(records);
  if (records_.version() != old_ver) {
    commitState();
  }

  scheduleCompaction();
}

void Partition::scheduleCompaction() {
  auto now = WallClock::unixMicros();
  auto interval = config_->compaction_interval();
  auto last = last_compaction_.unixMicros();
  uint64_t compaction_delay = 0;

  if (last + interval > now) {
    compaction_delay = (last + interval) - now;
  }

  node_->compactionq.insert(this, now + compaction_delay);
}

void Partition::compact() {
  std::unique_lock<std::mutex> lk(mutex_);
  last_compaction_ = DateTime::now();
  lk.unlock();

  fnord::logDebug(
      "tsdb",
      "Compacting partition; stream='$0' partition='$1'",
      stream_key_,
      key_.toString());

  Set<String> deleted_files;
  records_.compact(&deleted_files);

  lk.lock();
  commitState();
  lk.unlock();

  node_->replicationq.insert(this, WallClock::unixMicros());

  for (const auto& f : deleted_files) {
    FileUtil::rm(FileUtil::joinPaths(node_->db_path, f));
  }
}

void Partition::replicate() {
  std::unique_lock<std::mutex> lk(replication_mutex_);

  auto cur_offset = records_.lastOffset();
  auto replicas = node_->replication_scheme->replicasFor(
      String((char*) key_.data(), key_.size()));

  bool dirty = false;
  bool needs_replication = false;
  bool has_error = false;

  for (const auto& r : replicas) {
    auto& off = repl_offsets_[r.unique_id];

    if (off < cur_offset) {
      try {
        auto rep_offset = replicateTo(r.addr, off);
        dirty = true;
        off = rep_offset;
        if (off < cur_offset) {
          needs_replication = true;
        }
      } catch (const std::exception& e) {
        has_error = true;

        fnord::logError(
          "tsdb.replication",
          e,
          "Error while replicating stream '$0' to '$1'",
          stream_key_,
          r.addr);
      }
    }
  }

  for (auto cur = repl_offsets_.begin(); cur != repl_offsets_.end(); ) {
    bool found;
    for (const auto& r : replicas) {
      if (r.unique_id == cur->first) {
        found = true;
        break;
      }
    }

    if (found) {
      ++cur;
    } else {
      cur = repl_offsets_.erase(cur);
      dirty = true;
    }
  }

  if (dirty) {
    std::unique_lock<std::mutex> lk(mutex_);
    commitState();
  }

  if (needs_replication) {
    node_->replicationq.insert(this, WallClock::unixMicros());
  } else if (has_error) {
    node_->replicationq.insert(
        this,
        WallClock::unixMicros() + 30 * kMicrosPerSecond);
  }
}

uint64_t Partition::replicateTo(const String& addr, uint64_t offset) {
  size_t batch_size = 1024;
  util::BinaryMessageWriter batch;

  auto start_offset = records_.firstOffset();
  if (start_offset > offset) {
    offset = start_offset;
  }

  size_t n = 0;
  records_.fetchRecords(offset, batch_size, [this, &batch, &n] (
      const SHA1Hash& record_id,
      const void* record_data,
      size_t record_size) {
    ++n;

    batch.append(record_id.data(), record_id.size());
    batch.appendVarUInt(record_size);
    batch.append(record_data, record_size);
  });

  fnord::logDebug(
      "tsdb.replication",
      "Replicating to $0; stream='$1' partition='$2' offset=$3",
      addr,
      stream_key_,
      key_.toString(),
      offset);

  URI uri(StringUtil::format(
      "http://$0/tsdb/replicate?stream=$1&chunk=$2",
      addr,
      URI::urlEncode(stream_key_),
      URI::urlEncode(key_.toString())));

  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addHeader("Content-Type", "application/fnord-msg");
  req.addBody(batch.data(), batch.size());

  auto res = node_->http->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 201) {
    RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
  }

  return offset + n;
}

Vector<String> Partition::listFiles() const {
  return records_.listDatafiles();
}

PartitionInfo Partition::partitionInfo() const {
  PartitionInfo pi;
  pi.set_partition_key(key_.toString());
  pi.set_stream_key(stream_key_);
  pi.set_checksum(records_.checksum().toString());
  pi.set_exists(true);
  return pi;
}

void Partition::commitState() {
  PartitionState state;
  state.record_state = records_.getState();
  state.stream_key = stream_key_;
  state.repl_offsets = repl_offsets_;

  util::BinaryMessageWriter buf;
  state.encode(&buf);

  auto txn = node_->db->startTransaction(false);
  txn->update(db_key_.data(), db_key_.size(), buf.data(), buf.size());
  txn->commit();
}

void PartitionState::encode(
    util::BinaryMessageWriter* writer) const {
  writer->appendLenencString(stream_key);
  record_state.encode(writer);

  writer->appendVarUInt(repl_offsets.size());
  for (const auto& ro : repl_offsets) {
    writer->appendVarUInt(ro.first);
    writer->appendVarUInt(ro.second);
  }

  writer->appendVarUInt(record_state.version);
}

void PartitionState::decode(util::BinaryMessageReader* reader) {
  stream_key = reader->readLenencString();
  record_state.decode(reader);

  auto nrepl_offsets = reader->readVarUInt();
  for (int i = 0; i < nrepl_offsets; ++i) {
    auto id = reader->readVarUInt();
    auto off = reader->readVarUInt();
    repl_offsets.emplace(id, off);
  }

  record_state.version = reader->readVarUInt();
}

}
