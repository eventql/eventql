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
#include <fnord-cstable/CSTableBuilder.h>
#include <fnord-cstable/CSTableWriter.h>
#include <fnord-cstable/CSTableReader.h>
#include <fnord-cstable/UInt64ColumnWriter.h>
#include <fnord-cstable/UInt64ColumnReader.h>
#include <fnord-cstable/RecordMaterializer.h>
#include <fnord-msg/MessageDecoder.h>
#include <fnord-tsdb/RecordSet.h>

namespace fnord {
namespace tsdb {

RecordSet::RecordSet(
    RefPtr<msg::MessageSchema> schema,
    const String& filename_prefix,
    RecordSetState state /* = RecordSetState{} */) :
    schema_(schema),
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
  std::unique_lock<std::mutex> lk(mutex_);
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
  auto snap = state_;
  lk.unlock();

  if (snap.old_commitlogs.size() == 0) {
    return;
  }

  cstable::CSTableBuilder outfile(schema_.get());
  cstable::UInt64ColumnWriter id_col(0, 0);

  Set<uint64_t> old_id_set;
  Set<uint64_t> new_id_set;

  bool rewrite_last =
      snap.datafiles.size() > 0 &&
      FileUtil::size(snap.datafiles.back().first) < max_datafile_size_;

  for (int j = 0; j < snap.datafiles.size(); ++j) {
    cstable::CSTableReader reader(snap.datafiles[j].first);
    cstable::RecordMaterializer record_reader(schema_.get(), &reader);

    auto msgid_col_ref = reader.getColumnReader("__msgid");
    auto msgid_col = dynamic_cast<cstable::UInt64ColumnReader*>(msgid_col_ref.get());

    auto n = reader.numRecords();
    for (int i = 0; i < n; ++i) {
      uint64_t msgid;
      uint64_t r;
      uint64_t d;
      msgid_col->next(&r, &d, &msgid);
      old_id_set.emplace(msgid);

      if (!rewrite_last || j + 1 < snap.datafiles.size()) {
        continue;
      }

      msg::MessageObject record;
      record_reader.nextRecord(&record);

      outfile.addRecord(record);
      id_col.addDatum(0, 0, msgid);
    }
  }

  for (const auto& cl : snap.old_commitlogs) {
    loadCommitlog(cl, [this, &outfile, &old_id_set, &new_id_set, &id_col] (
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

      msg::MessageObject record;
      msg::MessageDecoder::decode(data, size, *schema_, &record);

      outfile.addRecord(record);
      id_col.addDatum(0, 0, id);
    });
  }

  auto outfile_path = filename_prefix_ + rnd_.hex64() + ".cst";
  auto outfile_nrecords = outfile.numRecords();
  cstable::CSTableWriter outfile_writer(
      outfile_path,
      outfile_nrecords);

  outfile.write(&outfile_writer);
  outfile_writer.addColumn("__msgid", &id_col);
  outfile_writer.commit();

  lk.lock();

  if (rewrite_last) {
    deleted_files->emplace(state_.datafiles.back().first);
    state_.datafiles.pop_back();
  }

  state_.datafiles.emplace_back(outfile_path, outfile_nrecords);

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

Set<uint64_t> RecordSet::listRecords() {
  Set<uint64_t> res;

  std::unique_lock<std::mutex> lk(mutex_);
  if (state_.datafiles.empty()) {
    return res;
  }

  auto datafiles = state_.datafiles;
  lk.unlock();

  for (const auto& datafile : datafiles) {
    cstable::CSTableReader reader(datafile.first);
    auto col = reader.getColumnReader("__msgid");

    void* data;
    size_t size;
    uint64_t r;
    uint64_t d;
    for (int i = 0; i < reader.numRecords(); ++i) {
      col->next(&r, &d, &data, &size);
      res.emplace(*((uint64_t*) data));
    }
  }

  return res;
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
    writer->appendVarUInt(d.second);
    writer->appendLenencString(d.first);
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
    auto fname = reader->readLenencString();
    datafiles.emplace_back(fname, num_records);
  }

  auto num_commitlogs = reader->readVarUInt();
  for (size_t i = 0; i < num_commitlogs; ++i) {
    auto fname = reader->readLenencString();
    old_commitlogs.emplace(fname);
  }
}

} // namespace tsdb
} // namespace fnord

