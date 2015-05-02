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
#include <fnord-base/random.h>
#include <fnord-base/option.h>
#include <fnord-base/io/file.h>
#include <fnord-base/io/mmappedfile.h>
#include <fnord-msg/MessageSchema.h>

namespace fnord {
namespace tsdb {

class RecordSet {
public:

  struct RecordSetState {
    RecordSetState();

    Option<String> datafile;
    Option<String> commitlog;
    uint64_t commitlog_size;
    Vector<String> old_commitlogs;
  };

  RecordSet(
      RefPtr<msg::MessageSchema> schema,
      const String& filename_prefix,
      RecordSetState state = RecordSetState{});

  void addRecord(uint64_t record_id, const Buffer& message);

  void fetchRecord(uint64_t record_id, Buffer* message);

  Vector<uint32_t> listRecords();

  RecordSetState getState() const;
  size_t commitlogSize() const;

  void rollCommitlog();
  void compact();

protected:

  void loadCommitlog(
      const String& filename,
      Function<void (uint64_t, const void*, size_t)> fn);

  RefPtr<msg::MessageSchema> schema_;
  String filename_prefix_;
  RecordSetState state_;
  mutable std::mutex mutex_;
  Random rnd_;
  Set<uint64_t> commitlog_ids_;
};

} // namespace tdsb
} // namespace fnord

#endif
