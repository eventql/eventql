/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-tsdb/StreamChunk.h>
#include <fnord-base/io/fileutil.h>
#include <fnord-base/uri.h>
#include <fnord-base/util/Base64.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-base/wallclock.h>
#include <fnord-msg/MessageEncoder.h>

namespace fnord {
namespace tsdb {

RefPtr<StreamChunk> StreamChunk::create(
    const String& streamchunk_key,
    const String& stream_key,
    RefPtr<StreamProperties> config,
    TSDBNodeRef* node) {
  return RefPtr<StreamChunk>(
      new StreamChunk(
          streamchunk_key,
          stream_key,
          config,
          node));
}

RefPtr<StreamChunk> StreamChunk::reopen(
    const String& stream_key,
    const StreamChunkState& state,
    RefPtr<StreamProperties> config,
    TSDBNodeRef* node) {
  return RefPtr<StreamChunk>(
      new StreamChunk(
          stream_key,
          state,
          config,
          node));
}

String StreamChunk::streamChunkKeyFor(
    const String& stream_key,
    DateTime time) {
  util::BinaryMessageWriter buf(stream_key.size() + 32);

  buf.append(stream_key.data(), stream_key.size());
  buf.appendUInt8(27);
  buf.appendVarUInt(time.unixMicros());

  return String((char *) buf.data(), buf.size());
}

String StreamChunk::streamChunkKeyFor(
    const String& stream_key,
    DateTime time,
    const StreamProperties& properties) {
  auto cs = properties.chunk_size.microseconds();

  return streamChunkKeyFor(
      stream_key,
      (time.unixMicros() / cs) * cs / kMicrosPerSecond);
}

Vector<String> StreamChunk::streamChunkKeysFor(
    const String& stream_key,
    DateTime from,
    DateTime until,
    const StreamProperties& properties) {
  auto cs = properties.chunk_size.microseconds();
  auto first_chunk = (from.unixMicros() / cs) * cs;
  auto last_chunk = (until.unixMicros() / cs) * cs;

  Vector<String> res;
  for (auto t = first_chunk; t <= last_chunk; t += cs) {
    res.emplace_back(streamChunkKeyFor(stream_key, t, properties));
  }

  return res;
}

StreamChunk::StreamChunk(
    const String& streamchunk_key,
    const String& stream_key,
    RefPtr<StreamProperties> config,
    TSDBNodeRef* node) :
    stream_key_(stream_key),
    key_(streamchunk_key),
    config_(config),
    node_(node),
    records_(
        FileUtil::joinPaths(
            node->db_path,
            StringUtil::stripShell(stream_key) + ".")),
    last_compaction_(0) {
  records_.setMaxDatafileSize(config_->max_datafile_size);
}

StreamChunk::StreamChunk(
    const String& streamchunk_key,
    const StreamChunkState& state,
    RefPtr<StreamProperties> config,
    TSDBNodeRef* node) :
    stream_key_(state.stream_key),
    key_(streamchunk_key),
    config_(config),
    node_(node),
    records_(
        FileUtil::joinPaths(
            node->db_path,
            StringUtil::stripShell(state.stream_key) + "."),
        state.record_state),
    repl_offsets_(state.repl_offsets),
    last_compaction_(0) {
  scheduleCompaction();
  node_->compactionq.insert(this, WallClock::unixMicros());
  node_->replicationq.insert(this, WallClock::unixMicros());
  node_->indexq.insert(this, WallClock::unixMicros());
  records_.setMaxDatafileSize(config_->max_datafile_size);
}

void StreamChunk::insertRecord(
    uint64_t record_id,
    const Buffer& record) {
  std::unique_lock<std::mutex> lk(mutex_);

  auto old_ver = records_.version();
  records_.addRecord(record_id, record);
  if (records_.version() != old_ver) {
    commitState();
  }

  scheduleCompaction();
}

void StreamChunk::insertRecords(const Vector<RecordRef>& records) {
  std::unique_lock<std::mutex> lk(mutex_);

  auto old_ver = records_.version();
  records_.addRecords(records);
  if (records_.version() != old_ver) {
    commitState();
  }

  scheduleCompaction();
}

void StreamChunk::scheduleCompaction() {
  auto now = WallClock::unixMicros();
  auto interval = config_->compaction_interval.microseconds();
  auto last = last_compaction_.unixMicros();
  uint64_t compaction_delay = 0;

  if (last + interval > now) {
    compaction_delay = (last + interval) - now;
  }

  node_->compactionq.insert(this, now + compaction_delay);
}

void StreamChunk::compact() {
  std::unique_lock<std::mutex> lk(mutex_);
  last_compaction_ = DateTime::now();
  lk.unlock();

  Set<String> deleted_files;
  records_.compact(&deleted_files);

  lk.lock();
  commitState();
  lk.unlock();

  node_->replicationq.insert(this, WallClock::unixMicros());
  node_->indexq.insert(this, WallClock::unixMicros());

  for (const auto& f : deleted_files) {
    FileUtil::rm(f);
  }
}

void StreamChunk::replicate() {
  std::unique_lock<std::mutex> lk(replication_mutex_);

  auto cur_offset = records_.lastOffset();
  auto replicas = node_->replication_scheme->replicasFor(key_);
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

uint64_t StreamChunk::replicateTo(const String& addr, uint64_t offset) {
  size_t batch_size = 1024;
  util::BinaryMessageWriter batch;

  auto start_offset = records_.firstOffset();
  if (start_offset > offset) {
    offset = start_offset;
  }

  size_t n = 0;
  records_.fetchRecords(offset, batch_size, [this, &batch, &n] (
      uint64_t record_id,
      const void* record_data,
      size_t record_size) {
    ++n;

    batch.appendUInt64(record_id);
    batch.appendVarUInt(record_size);
    batch.append(record_data, record_size);
  });

  String encoded_key;
  util::Base64::encode(key_, &encoded_key);

  fnord::logDebug(
      "tsdb.replication",
      "Replicating to $0; stream='$1' chunk='$2' offset=$3",
      addr,
      stream_key_,
      encoded_key,
      offset);

  URI uri(StringUtil::format(
      "http://$0/tsdb/replicate?stream=$1&chunk=$2",
      addr,
      URI::urlEncode(stream_key_),
      URI::urlEncode(encoded_key)));

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

void StreamChunk::buildIndexes() {
  std::unique_lock<std::mutex> lk(indexbuild_mutex_);

  for (const auto& dset : config_->derived_datasets) {
    buildDerivedDataset(dset);
  }
}

void StreamChunk::buildDerivedDataset(RefPtr<DerivedDataset> dset) {
  String dset_key = "\x1b";
  dset_key.append(key_);
  dset_key.append("~" + dset->name());

  DerivedDatasetState old_state;
  {
    auto txn = node_->db->startTransaction(true);
    auto buf = txn->get(dset_key);
    txn->abort();
    if (!buf.isEmpty()) {
      util::BinaryMessageReader reader(buf.get().data(), buf.get().size());
      old_state.decode(&reader);
    }
  }

  auto cur_offset = records_.lastOffset();
  if (old_state.last_offset >= cur_offset) {
    return;
  }

  DerivedDatasetState new_state;
  new_state.last_offset = cur_offset;

  Set<String> delete_after_commit;
  try {
    dset->update(
        &records_,
        old_state.last_offset,
        cur_offset,
        old_state.state,
        &new_state.state,
        &delete_after_commit);
  } catch (const std::exception& e) {
    fnord::logError(
        "fnord.tsdb",
        e,
        "error while building derived dataset '$0'",
        dset->name());

    return;
  }

  {
    util::BinaryMessageWriter buf;
    new_state.encode(&buf);

    auto txn = node_->db->startTransaction(false);
    txn->update(dset_key.data(), dset_key.size(), buf.data(), buf.size());
    txn->commit();
  }

  for (const auto& f : delete_after_commit) {
    FileUtil::rm(f);
  }
}

Buffer StreamChunk::fetchDerivedDataset(const String& dataset_name) {
  String dset_key = "\x1b";
  dset_key.append(key_);
  dset_key.append("~" + dataset_name);
  DerivedDatasetState dset;

  auto txn = node_->db->startTransaction(true);
  auto buf = txn->get(dset_key);
  txn->abort();

  if (buf.isEmpty()) {
    return Buffer{};
  } else {
    util::BinaryMessageReader reader(buf.get().data(), buf.get().size());
    dset.decode(&reader);
    return dset.state;
  }
}

Vector<String> StreamChunk::listFiles() const {
  return records_.listDatafiles();
}

void StreamChunk::commitState() {
  StreamChunkState state;
  state.record_state = records_.getState();
  state.stream_key = stream_key_;
  state.repl_offsets = repl_offsets_;

  util::BinaryMessageWriter buf;
  state.encode(&buf);

  auto txn = node_->db->startTransaction(false);
  txn->update(key_.data(), key_.size(), buf.data(), buf.size());
  txn->commit();
}

void StreamChunkState::encode(
    util::BinaryMessageWriter* writer) const {
  writer->appendLenencString(stream_key);
  record_state.encode(writer);

  writer->appendVarUInt(repl_offsets.size());
  for (const auto& ro : repl_offsets) {
    writer->appendVarUInt(ro.first);
    writer->appendVarUInt(ro.second);
  }

  writer->appendVarUInt(0);
}

void StreamChunkState::decode(util::BinaryMessageReader* reader) {
  stream_key = reader->readLenencString();
  record_state.decode(reader);

  auto nrepl_offsets = reader->readVarUInt();
  for (int i = 0; i < nrepl_offsets; ++i) {
    auto id = reader->readVarUInt();
    auto off = reader->readVarUInt();
    repl_offsets.emplace(id, off);
  }

  auto nderived_ds = reader->readVarUInt();
  for (int i = 0; i < nderived_ds; ++i) {
  }
}

}
}
