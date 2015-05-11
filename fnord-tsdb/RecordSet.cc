/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/io/fileutil.h>
#include <fnord-base/io/mmappedfile.h>
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-sstable/sstablewriter.h>
#include <fnord-sstable/sstablereader.h>
#include <fnord-tsdb/RecordSet.h>

namespace fnord {
namespace tsdb {

RecordSet::RecordSet(
    const String& filename_prefix,
    RecordSetState state /* = RecordSetState{} */) :
    filename_prefix_(filename_prefix),
    state_(state),
    max_datafile_size_(kDefaultMaxDatafileSize),
    version_(0) {
  auto id_index_fn = [this] (uint64_t id, const void* data, size_t size) {
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
  return version_;
}

size_t RecordSet::commitlogSize() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return commitlog_ids_.size();
}

void RecordSet::addRecord(uint64_t record_id, const Buffer& message) {
  util::BinaryMessageWriter buf;
  buf.appendUInt64(record_id);
  buf.appendVarUInt(message.size());
  buf.append(message.data(), message.size());

  std::unique_lock<std::mutex> lk(mutex_);

  if (commitlog_ids_.count(record_id) > 0) {
    return;
  }

  String commitlog;
  uint64_t commitlog_size;
  if (state_.commitlog.isEmpty()) {
    commitlog = filename_prefix_ + rnd_.hex64() + ".log";
    commitlog_size = 0;
    ++version_;
  } else {
    commitlog = state_.commitlog.get();
    commitlog_size = state_.commitlog_size;
  }

  auto file = File::openFile(commitlog, File::O_WRITE | File::O_CREATEOROPEN);
  file.truncate(sizeof(uint64_t) + commitlog_size + buf.size());
  file.seekTo(sizeof(uint64_t) + commitlog_size);
  file.write(buf.data(), buf.size());

  // FIXPAUL fsync here for non order preserving storage?

  commitlog_size += buf.size();
  file.seekTo(0);
  file.write(&commitlog_size, sizeof(commitlog_size));

  state_.commitlog = Some(commitlog);
  state_.commitlog_size = commitlog_size;
  commitlog_ids_.emplace(record_id);
}

void RecordSet::rollCommitlog() {
  if (state_.commitlog.isEmpty()) {
    return;
  }

  auto old_log = state_.commitlog.get();
  FileUtil::truncate(old_log, state_.commitlog_size + sizeof(uint64_t));
  state_.old_commitlogs.emplace(old_log);
  state_.commitlog = None<String>();
  state_.commitlog_size = 0;
  ++version_;
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
  rollCommitlog();
  auto snap = state_;
  lk.unlock();

  if (snap.old_commitlogs.size() == 0) {
    return;
  }

  auto outfile_path = filename_prefix_ + rnd_.hex64() + ".sst";
  auto outfile = sstable::SSTableWriter::create(
      outfile_path + "~",
      sstable::IndexProvider{},
      nullptr,
      0);

  size_t outfile_nrecords = 0;
  size_t outfile_offset = 0;

  Set<uint64_t> old_id_set;
  Set<uint64_t> new_id_set;

  bool rewrite_last =
      snap.datafiles.size() > 0 &&
      FileUtil::size(snap.datafiles.back().filename) < max_datafile_size_;

  if (rewrite_last) {
    outfile_offset = snap.datafiles.back().offset;
  } else {
    outfile_offset = snap.datafiles.size() > 0 ?
        snap.datafiles.back().offset + snap.datafiles.back().num_records :
        0;
  }

  for (int j = 0; j < snap.datafiles.size(); ++j) {
    sstable::SSTableReader reader(snap.datafiles[j].filename);
    auto cursor = reader.getCursor();

    while (cursor->valid()) {
      void* key;
      size_t key_size;
      cursor->getKey(&key, &key_size);
      if (key_size != sizeof(uint64_t)) {
        RAISE(kRuntimeError, "invalid row");
      }

      uint64_t msgid = *((uint64_t*) key);
      old_id_set.emplace(msgid);

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
    loadCommitlog(cl, [this, &outfile, &old_id_set, &new_id_set, &outfile_nrecords] (
        uint64_t id,
        const void* data,
        size_t size) {
      if (new_id_set.count(id) > 0) {
        return;
      }

      new_id_set.emplace(id);

      if (old_id_set.count(id) > 0) {
        return;
      }

      outfile->appendRow(&id, sizeof(id), data, size);
      ++outfile_nrecords;
    });
  }

  if (outfile_nrecords == 0) {
    return;
  }

  outfile->finalize();
  FileUtil::mv(outfile_path + "~", outfile_path);

  lk.lock();

  if (rewrite_last) {
    deleted_files->emplace(state_.datafiles.back().filename);
    state_.datafiles.pop_back();
  }

  state_.datafiles.emplace_back(DatafileRef {
    .filename = outfile_path,
    .num_records = outfile_nrecords,
    .offset = outfile_offset
  });

  for (const auto& cl : snap.old_commitlogs) {
    state_.old_commitlogs.erase(cl);
    deleted_files->emplace(cl);
  }

  for (const auto& id : new_id_set) {
    commitlog_ids_.erase(id);
  }

  ++version_;
}

void RecordSet::loadCommitlog(
    const String& filename,
    Function<void (uint64_t, const void*, size_t)> fn) {
  io::MmappedFile mmap(File::openFile(filename, File::O_READ));
  util::BinaryMessageReader reader(mmap.data(), mmap.size());
  auto limit = *reader.readUInt64() + sizeof(uint64_t);

  while (reader.position() < limit) {
    auto id = *reader.readUInt64();
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
      Function<void (uint64_t record_id, const void* record_data, size_t record_size)> fn) {
  Set<uint64_t> res;

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

    sstable::SSTableReader reader(datafile.filename);
    auto cursor = reader.getCursor();

    while (cursor->valid()) {
      if (++o > offset) {
        void* key;
        size_t key_size;
        cursor->getKey(&key, &key_size);
        if (key_size != sizeof(uint64_t)) {
          RAISE(kRuntimeError, "invalid row");
        }

        uint64_t msgid = *((uint64_t*) key);

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

Set<uint64_t> RecordSet::listRecords() const {
  Set<uint64_t> res;

  std::unique_lock<std::mutex> lk(mutex_);
  if (state_.datafiles.empty()) {
    return res;
  }

  auto datafiles = state_.datafiles;
  lk.unlock();

  for (const auto& datafile : datafiles) {
    sstable::SSTableReader reader(datafile.filename);
    auto cursor = reader.getCursor();

    while (cursor->valid()) {
      void* key;
      size_t key_size;
      cursor->getKey(&key, &key_size);
      if (key_size != sizeof(uint64_t)) {
        RAISE(kRuntimeError, "invalid row");
      }

      res.emplace(*((uint64_t*) key));

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
    commitlog_size(0) {}

void RecordSet::RecordSetState::encode(
    util::BinaryMessageWriter* writer) const {
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
}

void RecordSet::RecordSetState::decode(util::BinaryMessageReader* reader) {
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
}

} // namespace tsdb
} // namespace fnord

