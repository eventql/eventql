/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/io/FileUtil.h>
#include <fnord-tsdb/RecordSet.h>

namespace fnord {
namespace tsdb {

RecordSet::RecordSet(
    RefPtr<msg::MessageSchema> schema,
    const String& filename_prefix) :
    schema_(schema),
    filename_prefix_(filename_prefix) {}

RecordSet::RecordSetState RecordSet::getState() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return state_;
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

  String commitlog;
  uint64_t commitlog_size;
  if (state_.commitlog.isEmpty()) {
    commitlog = filename_prefix_ + rnd_.hex64() + ".log";
    commitlog_size = 0;
  } else {
    commitlog = state_.commitlog.get();
    commitlog_size = state_.commitlog_size;
  }

  auto file = File::openFile(commitlog, File::O_WRITE | File::O_CREATEOROPEN);
  file.truncate(commitlog_size + buf.size());
  file.seekTo(commitlog_size);
  file.write(buf.data(), buf.size());

  state_.commitlog = Some(commitlog);
  state_.commitlog_size = commitlog_size + buf.size();
  commitlog_ids_.emplace(record_id);
}

void RecordSet::rollCommitlog() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (state_.commitlog.isEmpty()) {
    return;
  }

  auto old_log = state_.commitlog.get();
  FileUtil::truncate(old_log, state_.commitlog_size);
  state_.old_commitlogs.emplace_back(old_log);
  state_.commitlog = None<String>();
  state_.commitlog_size = 0;
}

RecordSet::RecordSetState::RecordSetState() : commitlog_size(0) {}

} // namespace tsdb
} // namespace fnord

