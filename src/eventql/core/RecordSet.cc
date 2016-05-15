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
#include <eventql/util/io/fileutil.h>
#include <eventql/util/io/mmappedfile.h>
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/io/sstable/SSTableEditor.h>
#include <eventql/io/sstable/sstablereader.h>
#include <eventql/core/RecordSet.h>

#include "eventql/eventql.h"

namespace eventql {

RecordRef::RecordRef(
    const SHA1Hash& _record_id,
    uint64_t _record_version,
    const Buffer& _record) :
    record_id(_record_id),
    record_version(_record_version),
    record(_record),
    is_update(false) {}

RecordSet::RecordSet(
    const String& datadir,
    const String& filename_prefix,
    RecordSetState state /* = RecordSetState{} */) :
    datadir_(datadir),
    filename_prefix_(filename_prefix),
    state_(state),
    max_datafile_size_(kDefaultMaxDatafileSize) {
  auto id_index_fn = [this] (const SHA1Hash& id, const void* data, size_t size) {
    commitlog_ids_.emplace(id);
  };

  for (const auto& o : state_.old_commitlogs) {
    loadCommitlog(o, id_index_fn);
  }

  if (!state.commitlog.isEmpty()) {
    loadCommitlog(state.commitlog.get(), id_index_fn);
  }
}

RecordSet::RecordSetState RecordSet::getState() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return state_;
}

size_t RecordSet::version() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return state_.version;
}

SHA1Hash RecordSet::checksum() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return state_.checksum;
}

size_t RecordSet::commitlogSize() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return commitlog_ids_.size();
}

void RecordSet::addRecord(const SHA1Hash& record_id, const Buffer& message) {

  util::BinaryMessageWriter buf;
  buf.append(record_id.data(), record_id.size());
  buf.appendVarUInt(message.size());
  buf.append(message.data(), message.size());

  std::unique_lock<std::mutex> lk(mutex_);

  if (commitlog_ids_.count(record_id) > 0) {
    return;
  }

  addRecords(buf);

  commitlog_ids_.emplace(record_id);
}

void RecordSet::addRecords(const Vector<RecordRef>& records) {
  std::unique_lock<std::mutex> lk(mutex_);

  util::BinaryMessageWriter buf;
  for (const auto& rec : records) {
    if (commitlog_ids_.count(rec.record_id) > 0) {
      continue;
    }

    buf.append(rec.record_id.data(), rec.record_id.size());
    buf.appendVarUInt(rec.record.size());
    buf.append(rec.record.data(), rec.record.size());
  }

  if (buf.size() == 0) {
    return;
  }

  addRecords(buf);

  for (const auto& rec : records) {
    commitlog_ids_.emplace(rec.record_id);
  }
}

// precondition: must hold lock
void RecordSet::addRecords(const util::BinaryMessageWriter& buf) {
  String commitlog;
  uint64_t commitlog_size;
  if (state_.commitlog.isEmpty()) {
    commitlog = filename_prefix_ + rnd_.hex64() + ".log";
    commitlog_size = 0;
    ++state_.version;
  } else {
    commitlog = state_.commitlog.get();
    commitlog_size = state_.commitlog_size;
  }

  auto commitlog_path = FileUtil::joinPaths(datadir_, commitlog);
  auto file = File::openFile(
      commitlog_path,
      File::O_WRITE | File::O_CREATEOROPEN);

  file.truncate(sizeof(uint64_t) + commitlog_size + buf.size());
  file.seekTo(sizeof(uint64_t) + commitlog_size);
  file.write(buf.data(), buf.size());

  // FIXPAUL fsync here for non order preserving storage?

  commitlog_size += buf.size();
  file.seekTo(0);
  file.write(&commitlog_size, sizeof(commitlog_size));

  state_.commitlog = Some(commitlog);
  state_.commitlog_size = commitlog_size;
}

void RecordSet::rollCommitlog() {
  std::unique_lock<std::mutex> lk(mutex_);
  rollCommitlogWithLock();
}

// precondition: must hold lock
void RecordSet::rollCommitlogWithLock() {
  if (state_.commitlog.isEmpty()) {
    return;
  }

  auto old_log = state_.commitlog.get();
  auto old_log_path = FileUtil::joinPaths(datadir_, old_log);
  FileUtil::truncate(old_log_path, state_.commitlog_size + sizeof(uint64_t));

  state_.old_commitlogs.emplace(old_log);
  state_.commitlog = None<String>();
  state_.commitlog_size = 0;
  ++state_.version;
}

void RecordSet::compact() {
  Set<String> deleted_files;
  compact(&deleted_files);
}

void RecordSet::compact(Set<String>* deleted_files) {
  std::unique_lock<std::mutex> compact_lk(compact_mutex_, std::defer_lock);
  if (!compact_lk.try_lock()) {
    return; // compaction is already running
  }

  std::unique_lock<std::mutex> lk(mutex_);
  rollCommitlogWithLock();
  auto snap = state_;
  lk.unlock();

  if (snap.old_commitlogs.size() == 0) {
    return;
  }

  auto outfile_name = filename_prefix_ + rnd_.hex64() + ".sst";
  auto outfile_path = FileUtil::joinPaths(datadir_, outfile_name);
  auto outfile = sstable::SSTableEditor::create(
      outfile_path + "~",
      sstable::IndexProvider{},
      nullptr,
      0);

  size_t outfile_nrecords = 0;
  size_t outfile_offset = 0;

  Set<SHA1Hash> id_set;
  Set<SHA1Hash> new_id_set;

  bool rewrite_last = false;
  if (snap.datafiles.size() > 0) {
    auto cur_datafile_size = FileUtil::size(
        FileUtil::joinPaths(datadir_, snap.datafiles.back().filename));

    if (cur_datafile_size < max_datafile_size_) {
      rewrite_last = true;
    }
  }

  if (rewrite_last) {
    outfile_offset = snap.datafiles.back().offset;
  } else {
    outfile_offset = snap.datafiles.size() > 0 ?
        snap.datafiles.back().offset + snap.datafiles.back().num_records :
        0;
  }

  for (int j = 0; j < snap.datafiles.size(); ++j) {
    auto sstable_path = FileUtil::joinPaths(
        datadir_,
        snap.datafiles[j].filename);

    sstable::SSTableReader reader(sstable_path);
    auto cursor = reader.getCursor();

    while (cursor->valid()) {
      void* key;
      size_t key_size;
      cursor->getKey(&key, &key_size);

      SHA1Hash msgid(key, key_size);
      id_set.emplace(msgid);

      if (rewrite_last && j + 1 == snap.datafiles.size()) {
        void* data;
        size_t data_size;
        cursor->getData(&data, &data_size);

        outfile->appendRow(key, key_size, data, data_size);
        ++outfile_nrecords;
      }

      if (!cursor->next()) {
        break;
      }
    }
  }

  for (const auto& cl : snap.old_commitlogs) {
    loadCommitlog(cl, [this, &outfile, &id_set, &new_id_set, &outfile_nrecords] (
        const SHA1Hash& id,
        const void* data,
        size_t size) {
      if (new_id_set.count(id) > 0) {
        return;
      }

      new_id_set.emplace(id);

      if (id_set.count(id) > 0) {
        return;
      }

      outfile->appendRow(id.data(), id.size(), data, size);
      ++outfile_nrecords;
    });
  }

  for (const auto& new_id : new_id_set) {
    id_set.emplace(new_id);
  }

  auto new_checksum = calculateChecksum(id_set);

  if (outfile_nrecords == 0) {
    FileUtil::rm(outfile_path + "~");
  } else {
    outfile->finalize();
    FileUtil::mv(outfile_path + "~", outfile_path);
  }

  lk.lock();

  if (rewrite_last) {
    deleted_files->emplace(state_.datafiles.back().filename);
    state_.datafiles.pop_back();
  }

  if (outfile_nrecords > 0) {
    state_.datafiles.emplace_back(DatafileRef {
      .filename = outfile_name,
      .num_records = outfile_nrecords,
      .offset = outfile_offset
    });
  }

  for (const auto& cl : snap.old_commitlogs) {
    state_.old_commitlogs.erase(cl);
    deleted_files->emplace(cl);
  }

  for (const auto& id : new_id_set) {
    commitlog_ids_.erase(id);
  }

  state_.checksum = new_checksum;
  ++state_.version;
}

void RecordSet::loadCommitlog(
    const String& filename,
    Function<void (const SHA1Hash&, const void*, size_t)> fn) {
  auto filepath = FileUtil::joinPaths(datadir_, filename);
  MmappedFile mmap(File::openFile(filepath, File::O_READ));
  util::BinaryMessageReader reader(mmap.data(), mmap.size());
  auto limit = *reader.readUInt64() + sizeof(uint64_t);

  while (reader.position() < limit) {
    auto id_data = reader.read(SHA1Hash::kSize);
    SHA1Hash id(id_data, SHA1Hash::kSize);
    auto len = reader.readVarUInt();
    auto data = reader.read(len);

    fn(id, data, len);
  }
}

uint64_t RecordSet::numRecords() const {
  uint64_t res = 0;

  std::unique_lock<std::mutex> lk(mutex_);
  for (const auto& df : state_.datafiles) {
    res += df.num_records;
  }

  return res;
}

uint64_t RecordSet::firstOffset() const {
  std::unique_lock<std::mutex> lk(mutex_);

  if (state_.datafiles.size() == 0) {
    return 0;
  } else {
    return state_.datafiles.front().offset;
  }
}

uint64_t RecordSet::lastOffset() const {
  std::unique_lock<std::mutex> lk(mutex_);

  if (state_.datafiles.size() == 0) {
    return 0;
  } else {
    return state_.datafiles.back().offset + state_.datafiles.back().num_records;
  }
}

void RecordSet::fetchRecords(
      uint64_t offset,
      uint64_t limit,
      Function<void (
          const SHA1Hash& record_id,
          const void* record_data,
          size_t record_size)> fn) {
  std::unique_lock<std::mutex> lk(mutex_);
  auto datafiles = state_.datafiles;
  lk.unlock();

  if (datafiles.size() == 0) {
    return;
  }

  size_t o = datafiles.front().offset;
  if (o > offset) {
    RAISEF(kRuntimeError, "offset was garbage collected: $0", offset);
  }

  size_t l = 0;
  for (const auto& datafile : datafiles) {
    if (o + datafile.num_records <= offset) {
      o += datafile.num_records;
      continue;
    }

    auto filepath = FileUtil::joinPaths(datadir_, datafile.filename);
    sstable::SSTableReader reader(filepath);
    auto cursor = reader.getCursor();

    while (cursor->valid()) {
      if (++o > offset) {
        void* key;
        size_t key_size;
        cursor->getKey(&key, &key_size);
        SHA1Hash msgid(key, key_size);

        void* data;
        size_t data_size;
        cursor->getData(&data, &data_size);
        fn(msgid, data, data_size);

        if (++l == limit) {
          return;
        }
      }

      if (!cursor->next()) {
        break;
      }
    }
  }
}

Set<SHA1Hash> RecordSet::listRecords() const {
  Set<SHA1Hash> res;

  std::unique_lock<std::mutex> lk(mutex_);
  if (state_.datafiles.empty()) {
    return res;
  }

  auto datafiles = state_.datafiles;
  lk.unlock();

  for (const auto& datafile : datafiles) {
    auto filepath = FileUtil::joinPaths(datadir_, datafile.filename);
    sstable::SSTableReader reader(filepath);
    auto cursor = reader.getCursor();

    while (cursor->valid()) {
      void* key;
      size_t key_size;
      cursor->getKey(&key, &key_size);

      res.emplace(key, key_size);

      if (!cursor->next()) {
        break;
      }
    }
  }

  return res;
}

Vector<String> RecordSet::listDatafiles() const {
  std::unique_lock<std::mutex> lk(mutex_);
  Vector<String> datafiles;

  for (const auto& df : state_.datafiles) {
    datafiles.emplace_back(df.filename);
  }

  return datafiles;
}

void RecordSet::setMaxDatafileSize(size_t size) {
  max_datafile_size_ = size;
}

RecordSet::RecordSetState::RecordSetState() :
    commitlog_size(0),
    version(0) {}

void RecordSet::RecordSetState::encode(
    util::BinaryMessageWriter* writer) const {
  writer->appendVarUInt(0x01);

  Set<String> all_commitlogs = old_commitlogs;
  if (!commitlog.isEmpty()) {
    all_commitlogs.emplace(commitlog.get());
  }

  writer->appendVarUInt(datafiles.size());
  for (const auto& d : datafiles) {
    writer->appendVarUInt(d.num_records);
    writer->appendVarUInt(d.offset);
    writer->appendLenencString(d.filename);
  }

  writer->appendVarUInt(all_commitlogs.size());
  for (const auto& cl : all_commitlogs) {
    writer->appendLenencString(cl);
  }

  writer->append(checksum.data(), checksum.size());
}

void RecordSet::RecordSetState::decode(util::BinaryMessageReader* reader) {
  auto v = reader->readVarUInt();
  (void) v;

  auto num_datafiles = reader->readVarUInt();
  for (size_t i = 0; i < num_datafiles; ++i) {
    auto num_records = reader->readVarUInt();
    auto offset = reader->readVarUInt();
    auto fname = reader->readLenencString();
    datafiles.emplace_back(DatafileRef {
      .filename = fname,
      .num_records = num_records,
      .offset = offset
    });
  }

  auto num_commitlogs = reader->readVarUInt();
  for (size_t i = 0; i < num_commitlogs; ++i) {
    auto fname = reader->readLenencString();
    old_commitlogs.emplace(fname);
  }

  checksum = SHA1Hash(reader->read(SHA1Hash::kSize), SHA1Hash::kSize);
}

// FIXPAUL this should use an incremental algo and should be O(1) in space
// instead of O(N)
SHA1Hash RecordSet::calculateChecksum(const Set<SHA1Hash>& id_set) const {
  Buffer id_set_concat;

  for (const auto& id : id_set) {
    id_set_concat.append(id.data(), id.size());
  }

  return SHA1::compute(id_set_concat);
}

} // namespace eventql

