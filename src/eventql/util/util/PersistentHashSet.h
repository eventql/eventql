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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/io/file.h>
#include <eventql/util/option.h>
#include <eventql/util/SHA1.h>
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/random.h>

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

