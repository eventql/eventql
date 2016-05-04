/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/io/file.h>
#include <stx/option.h>
#include <stx/SHA1.h>
#include <stx/util/binarymessagereader.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/random.h>

namespace stx {

class PersistentHashSet {
public:

  PersistentHashSet(const String& fpath);

  bool addRecordID(const SHA1Hash& record_id);
  void addRecordIDs(Set<SHA1Hash>* record_ids);

  bool hasRecordID(const SHA1Hash& record_id);

  Set<SHA1Hash> fetchRecordIDs();

protected:
  static const size_t kVersion;
  static const double kMaxFillFactor;
  static const double kGrowthFactor;
  static const size_t kInitialSlots;
  static const size_t kFetchIOBatchSize;
  static const size_t kProbeIOBatchSize;

  struct  __attribute__((packed)) FileHeader {
    uint8_t version;
    uint8_t unused[3];
    uint64_t nslots;
  };

  void scan(
      File* file,
      size_t nslots,
      Function<void (void* slot)> fn);

  bool lookup(
      File* file,
      size_t nslots,
      const SHA1Hash& record_id,
      size_t* insert_idx = nullptr);

  void grow(File* file);

  bool insert(
      File* file,
      const SHA1Hash& record_id);

  String fpath_;
  size_t nslots_;
  size_t nslots_used_;
  mutable std::mutex write_mutex_;
  mutable std::mutex read_mutex_;
};

} // namespace stx

