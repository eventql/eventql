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
#ifndef _FNORD_TSDB_MESSAGESET_H
#define _FNORD_TSDB_MESSAGESET_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/io/file.h>
#include <eventql/util/option.h>
#include <eventql/util/SHA1.h>
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/random.h>
#include <eventql/db/RecordRef.h>

#include "eventql/eventql.h"

namespace eventql {

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
