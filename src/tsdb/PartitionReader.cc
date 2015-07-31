/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/fnv.h>
#include <stx/io/fileutil.h>
#include <sstable/sstablereader.h>
#include <tsdb/PartitionReader.h>

using namespace stx;

namespace tsdb {

PartitionReader::PartitionReader(
    RefPtr<PartitionSnapshot> head) :
    snap_(head) {}

void PartitionReader::fetchRecords(Function<void (const Buffer& record)> fn) {
  fetchRecordsWithSampling(
      0,
      0,
      fn);
}

void PartitionReader::fetchRecordsWithSampling(
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
      uint64_t* key;
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

Option<RefPtr<VFSFile>> PartitionReader::fetchSecondaryIndex(
    const String& index) const {
  return None<RefPtr<VFSFile>>();
}

} // namespace tdsb

