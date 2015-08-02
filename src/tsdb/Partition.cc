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
#include <stx/io/fileutil.h>
#include <stx/uri.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/wallclock.h>
#include <stx/protobuf/MessageEncoder.h>
#include <stx/protobuf/MessageDecoder.h>
#include <stx/protobuf/msg.h>
#include <cstable/CSTableBuilder.h>
#include <sstable/sstablereader.h>

using namespace stx;

namespace tsdb {

RefPtr<Partition> Partition::create(
    const String& tsdb_namespace,
    RefPtr<Table> table,
    const SHA1Hash& partition_key,
    const String& db_path) {
  stx::logDebug(
      "tsdb",
      "Creating new partition; stream='$0' partition='$1'",
      table->name(),
      partition_key.toString());

  auto pdir = FileUtil::joinPaths(
      db_path,
      StringUtil::format(
          "$0/$1/$2",
          tsdb_namespace,
          SHA1::compute(table->name()).toString(),
          partition_key.toString()));

  FileUtil::mkdir_p(pdir);

  auto uuid = Random::singleton()->sha1();

  PartitionState state;
  state.set_tsdb_namespace(tsdb_namespace);
  state.set_partition_key(partition_key.data(), partition_key.size());
  state.set_table_key(table->name());
  state.set_uuid(uuid.data(), uuid.size());

  auto snap = mkRef(new PartitionSnapshot(state, pdir, 0));
  snap->writeToDisk();
  return new Partition(snap, table);
}

RefPtr<Partition> Partition::reopen(
    const String& tsdb_namespace,
    RefPtr<Table> table,
    const SHA1Hash& partition_key,
    const String& db_path) {
  auto pdir = FileUtil::joinPaths(
      db_path,
      StringUtil::format(
          "$0/$1/$2",
          tsdb_namespace,
          SHA1::compute(table->name()).toString(),
          partition_key.toString()));

  auto state = msg::decode<PartitionState>(
      FileUtil::read(FileUtil::joinPaths(pdir, "_snapshot")));

  auto nrecs = 0;
  const auto& files = state.sstable_files();
  for (const auto& f : files) {
    sstable::SSTableReader reader(FileUtil::joinPaths(pdir, f));
    nrecs += reader.countRows();
  }

  stx::logDebug(
      "tsdb",
      "Loading partition $0/$1/$2 ($3 records)",
      tsdb_namespace,
      table->name(),
      partition_key.toString(),
      nrecs);

  auto snap = mkRef(new PartitionSnapshot(state, pdir, nrecs));
  return new Partition(snap, table);
}

Partition::Partition(
    RefPtr<PartitionSnapshot> head,
    RefPtr<Table> table) :
    head_(head),
    table_(table),
    writer_(new PartitionWriter(&head_)) {}

SHA1Hash Partition::uuid() const {
  auto snap = head_.getSnapshot();
  return snap->uuid();
}

RefPtr<PartitionWriter> Partition::getWriter() {
  return writer_;
}

RefPtr<PartitionReader> Partition::getReader() {
  return new PartitionReader(head_.getSnapshot());
}

RefPtr<PartitionSnapshot> Partition::getSnapshot() {
  return head_.getSnapshot();
}

RefPtr<Table> Partition::getTable() {
  return table_;
}

PartitionInfo Partition::getInfo() const {
  auto snap = head_.getSnapshot();

  auto checksum = SHA1::compute(
      StringUtil::format("$0~$1", snap->key.toString(), snap->nrecs));

  PartitionInfo pi;
  pi.set_partition_key(snap->key.toString());
  pi.set_stream_key(table_->name());
  pi.set_checksum(checksum.toString());
  pi.set_exists(true);
  return pi;
}

//void Partition::scheduleCompaction() {
//  auto now = WallClock::unixMicros();
//  auto interval = table_->compactionInterval().microseconds();
//  auto last = last_compaction_.unixMicros();
//  uint64_t compaction_delay = 0;
//
//  if (last + interval > now) {
//    compaction_delay = (last + interval) - now;
//  }
//
//  node_->compactionq.insert(this, now + compaction_delay);
//}

/*
void Partition::compact() {
  std::unique_lock<std::mutex> lk(mutex_);
  last_compaction_ = UnixTime::now();
  lk.unlock();

  stx::logDebug(
      "tsdb",
      "Compacting partition; stream='$0' partition='$1'",
      stream_key_,
      key_.toString());

  try {
    Set<String> deleted_files;
    records_.compact(&deleted_files);

    lk.lock();
    auto version = records_.checksum();
    auto cstable_file = cstable_file_;
    auto cstable_version = cstable_version_;
    auto sstable_files = records_.listDatafiles();
    lk.unlock();

    if (!(cstable_version == version)) {
      auto new_cstable_file = SHA1::compute(
          key_.toString() + version.toString()).toString() + ".cst";
      buildCSTable(sstable_files, new_cstable_file);

      if (cstable_file != new_cstable_file) {
        deleted_files.emplace(cstable_file);
      }

      cstable_file = new_cstable_file;
      cstable_version = version;
    }

    lk.lock();
    cstable_file_ = cstable_file;
    cstable_version_ = cstable_version;
    commitState();
    lk.unlock();

    node_->replicationq.insert(this, WallClock::unixMicros());

    for (const auto& f : deleted_files) {
      FileUtil::rm(FileUtil::joinPaths(node_->db_path, f));
    }
  } catch (const std::exception& e) {
    stx::logError(
        "tsdb",
        e,
        "error while compacting partition $0/$1",
        stream_key_,
        key_.toString());
  }
}
*/

/*
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

        stx::logError(
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

  stx::logDebug(
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
*/

}
