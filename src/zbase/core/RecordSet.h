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
#include <stx/stdtypes.h>
#include <stx/io/file.h>
#include <stx/option.h>
#include <stx/SHA1.h>
#include <stx/util/binarymessagereader.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/random.h>
#include <zbase/core/RecordRef.h>

using namespace stx;

namespace tsdb {

class RecordSet {
public:
  static const size_t kDefaultMaxDatafileSize = 1024 * 1024 * 128;

  struct DatafileRef {
    String filename;
    uint64_t num_records;
    uint64_t offset;
  };

  struct RecordSetState {
    RecordSetState();

    Vector<DatafileRef> datafiles;
    Option<String> commitlog;
    uint64_t commitlog_size;
    Set<String> old_commitlogs;
    size_t version;
    SHA1Hash checksum;

    void encode(util::BinaryMessageWriter* writer) const;
    void decode(util::BinaryMessageReader* reader);
  };

  RecordSet(
      const String& datadir,
      const String& filename_prefix,
      RecordSetState state = RecordSetState{});

  void addRecord(const SHA1Hash& record_id, const Buffer& record);
  void addRecords(const RecordRef& record);
  void addRecords(const Vector<RecordRef>& records);

  void fetchRecords(
      uint64_t offset,
      uint64_t limit,
      Function<void (
          const SHA1Hash& record_id,
          const void* record_data,
          size_t record_size)> fn);

  Set<SHA1Hash> listRecords() const;
  uint64_t numRecords() const;

  uint64_t firstOffset() const;
  uint64_t lastOffset() const;

  RecordSetState getState() const;
  Vector<String> listDatafiles() const;

  size_t version() const;
  SHA1Hash checksum() const;
  size_t commitlogSize() const;

  void compact();
  void compact(Set<String>* deleted_files);

  void setMaxDatafileSize(size_t size);

  void rollCommitlog();

protected:

  void rollCommitlogWithLock();
  void addRecords(const util::BinaryMessageWriter& buf);

  void loadCommitlog(
      const String& filename,
      Function<void (const SHA1Hash&, const void*, size_t)> fn);

  SHA1Hash calculateChecksum(const Set<SHA1Hash>& id_set) const;

  String datadir_;
  String filename_prefix_;
  RecordSetState state_;
  mutable std::mutex compact_mutex_;
  mutable std::mutex mutex_;
  Random rnd_;
  Set<SHA1Hash> commitlog_ids_;
  size_t max_datafile_size_;
};

} // namespace tdsb

#endif
