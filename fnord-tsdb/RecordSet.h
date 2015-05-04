/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_MESSAGESET_H
#define _FNORD_TSDB_MESSAGESET_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/io/file.h>
#include <fnord-base/io/mmappedfile.h>
#include <fnord-base/option.h>
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-base/random.h>
#include <fnord-msg/MessageSchema.h>

namespace fnord {
namespace tsdb {

class RecordSet {
public:
  static const size_t kDefaultMaxDatafileSize = 1024 * 1024 * 128;

  struct RecordSetState {
    RecordSetState();

    Vector<Pair<String, uint64_t>> datafiles;
    Option<String> commitlog;
    uint64_t commitlog_size;
    Set<String> old_commitlogs;

    void encode(util::BinaryMessageWriter* writer) const;
    void decode(util::BinaryMessageReader* reader);
  };

  RecordSet(
      RefPtr<msg::MessageSchema> schema,
      const String& filename_prefix,
      RecordSetState state = RecordSetState{});

  void addRecord(uint64_t record_id, const Buffer& message);

  void fetchRecord(uint64_t record_id, Buffer* message);

  Set<uint64_t> listRecords() const;

  RecordSetState getState() const;
  Vector<String> listDatafiles() const;

  size_t version() const;
  size_t commitlogSize() const;

  void rollCommitlog();
  void compact();
  void compact(Set<String>* deleted_files);

  void setMaxDatafileSize(size_t size);

protected:

  void loadCommitlog(
      const String& filename,
      Function<void (uint64_t, const void*, size_t)> fn);

  size_t version_;
  RefPtr<msg::MessageSchema> schema_;
  String filename_prefix_;
  RecordSetState state_;
  mutable std::mutex compact_mutex_;
  mutable std::mutex mutex_;
  Random rnd_;
  Set<uint64_t> commitlog_ids_;
  size_t max_datafile_size_;
};

} // namespace tdsb
} // namespace fnord

#endif
