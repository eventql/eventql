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
#include <eventql/util/fnv.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/protobuf/MessageDecoder.h>
#include <eventql/io/sstable/sstablereader.h>
#include <eventql/core/LogPartitionReader.h>
#include <eventql/core/Table.h>

#include "eventql/eventql.h"

namespace eventql {

LogPartitionReader::LogPartitionReader(
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> head) :
    PartitionReader(head),
    table_(table) {}

void LogPartitionReader::fetchRecords(
    const Set<String>& required_columns,
    Function<void (const msg::MessageObject& record)> fn) {
  auto schema = table_->schema();

  fetchRecords(
      0,
      -1,
      [
        &fn,
        &schema
      ] (
        const SHA1Hash& record_id,
        const void* record_data,
        size_t record_size
      ) {
    msg::MessageObject msgobj;
    msg::MessageDecoder::decode(record_data, record_size, *schema, &msgobj);
    fn(msgobj);
  });
}

void LogPartitionReader::fetchRecords(
    size_t offset,
    size_t limit,
    Function<void (
        const SHA1Hash& record_id,
        const void* record_data,
        size_t record_size)> fn) {
  int64_t remaining = snap_->nrecs;
  const auto& files = snap_->state.sstable_files();
  for (const auto& f : files) {
    auto fpath = FileUtil::joinPaths(snap_->base_path, f);
    sstable::SSTableReader reader(fpath);

    auto nrows = reader.countRows();
    if (offset > nrows) {
      offset -= nrows;
      remaining -= nrows;
      continue;
    }

    if (remaining <= 0) {
      return;
    }

    auto cursor = reader.getCursor();
    while (cursor->valid()) {
      void* key;
      size_t key_size;
      cursor->getKey(&key, &key_size);
      if (key_size != SHA1Hash::kSize) {
        RAISE(kRuntimeError, "invalid row");
      }

      void* data;
      size_t data_size;
      cursor->getData(&data, &data_size);

      if (offset > 0) {
        --offset;
      } else {
        fn(SHA1Hash(key, key_size), data, data_size);

        if (limit != size_t(-1) && --limit == 0) {
          return;
        }
      }

      if (--remaining <= 0) {
        return;
      }

      if (!cursor->next()) {
        break;
      }
    }
  }
}

void LogPartitionReader::fetchRecords(Function<void (const Buffer& record)> fn) {
  fetchRecordsWithSampling(
      0,
      0,
      fn);
}

void LogPartitionReader::fetchRecordsWithSampling(
    size_t sample_modulo,
    size_t sample_index,
    Function<void (const Buffer& record)> fn) {
  FNV<uint64_t> fnv;

  auto nrows = snap_->nrecs;
  const auto& files = snap_->state.sstable_files();
  for (const auto& f : files) {
    auto fpath = FileUtil::joinPaths(snap_->base_path, f);
    sstable::SSTableReader reader(fpath);
    auto cursor = reader.getCursor();

    while (cursor->valid()) {
      void* key;
      size_t key_size;
      cursor->getKey((void**) &key, &key_size);
      if (key_size != SHA1Hash::kSize) {
        RAISE(kRuntimeError, "invalid row");
      }

      if (sample_modulo == 0 ||
          (fnv.hash(key, key_size) % sample_modulo == sample_index)) {
        void* data;
        size_t data_size;
        cursor->getData(&data, &data_size);

        fn(Buffer(data, data_size));
      }

      if (--nrows == 0) {
        return;
      }

      if (!cursor->next()) {
        break;
      }
    }
  }
}

SHA1Hash LogPartitionReader::version() const {
  return SHA1::compute(StringUtil::toString(snap_->nrecs));
}

} // namespace tdsb

