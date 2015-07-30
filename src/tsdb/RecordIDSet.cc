/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <sys/mman.h>
#include <unistd.h>
#include <tsdb/RecordIDSet.h>
#include <stx/io/fileutil.h>
#include <stx/io/mmappedfile.h>
#include <stx/fnv.h>

using namespace stx;

namespace tsdb {

const double RecordIDSet::kMaxFillFactor = 0.5f;
const double RecordIDSet::kGrowthFactor = 2.0f;
const size_t RecordIDSet::kInitialSlots = 512;

#define IS_SLOT_EMPTY(slot) (\
    *((uint64_t*) (slot)) == 0 && \
    memcmp( \
        (slot), \
        (slot) + sizeof(uint64_t), \
        SHA1Hash::kSize - sizeof(uint64_t)) == 0)

RecordIDSet::RecordIDSet(
    const String& filepath) :
    fpath_(filepath),
    nslots_(0),
    nslots_used_(-1) {
  if (FileUtil::exists(filepath)) {
    reopenFile();
  }
}

void RecordIDSet::addRecordID(const SHA1Hash& record_id) {
  if (nslots_used_ == size_t(-1)) {
    countUsedSlots();
  }

  if (nslots_used_ + 1 > nslots_ * kMaxFillFactor) {
    grow();
  }

  size_t insert_idx = 0;
  if (!lookup(record_id, &insert_idx)) {
    auto file = File::openFile(fpath_, File::O_WRITE);
    file.seekTo(sizeof(FileHeader) + insert_idx * SHA1Hash::kSize);
    file.write(record_id.data(), SHA1Hash::kSize);
    ++nslots_used_;
  }
}

bool RecordIDSet::hasRecordID(const SHA1Hash& record_id) {
  if (nslots_ == 0) {
    return false;
  }

  return lookup(record_id);
}

Set<SHA1Hash> RecordIDSet::fetchRecordIDs() {
  Set<SHA1Hash> ids;
  if (nslots_ == 0) {
    return ids;
  }

  scan([&ids] (void* slot) {
    ids.emplace(SHA1Hash(slot, SHA1Hash::kSize));
  });

  return ids;
}

bool RecordIDSet::lookup(
    const SHA1Hash& record_id,
    size_t* insert_idx /* = nullptr */) {
  auto file = File::openFile(fpath_, File::O_READ);

  FNV<uint64_t> fnv;
  auto h = fnv.hash(record_id.data(), record_id.size());

  static const size_t kBatchSize = 2;
  Buffer buf(kBatchSize * SHA1Hash::kSize);
  size_t buf_offset = -1;
  size_t buf_size = 0;

  for (size_t i = 0; i < nslots_; ++i) {
    auto idx = (h + i) % nslots_;

    if (!buf_size || idx >= buf_offset + buf_size || idx < buf_offset) {
      file.seekTo(sizeof(FileHeader) + idx * SHA1Hash::kSize);
      auto nread = file.read(buf.data(), buf.size());
      if (nread < SHA1Hash::kSize) {
        RAISE(kRuntimeError, "error while reading from file");;
      }

      buf_offset = idx;
      buf_size = nread / SHA1Hash::kSize;
    }

    auto slot = (char *) buf.data() + (idx - buf_offset) * SHA1Hash::kSize;

    if (memcmp(slot, record_id.data(), SHA1Hash::kSize) == 0) {
      return true;
    }

    if (IS_SLOT_EMPTY(slot)) {
      if (insert_idx) {
        *insert_idx = idx;
      }
      return false;
    }
  }

  RAISE(kIllegalStateError, "set is full");
}


void RecordIDSet::scan(Function<void (void* slot)> fn) {
  auto file = File::openFile(fpath_, File::O_READ);
  file.seekTo(sizeof(FileHeader));

  static const size_t kBatchSize = 256;
  Buffer buf(kBatchSize * SHA1Hash::kSize);
  auto nbatches = (nslots_ + kBatchSize - 1) / kBatchSize;
  for (size_t b = 0; b < nbatches; ++b) {
    auto nread = file.read(buf.data(), buf.size());
    if (nread == -1) {
      break;
    }

    for (size_t pos = 0; pos < SHA1Hash::kSize * kBatchSize;
        pos += SHA1Hash::kSize) {
      auto slot = static_cast<char*>(buf.data()) + pos;
      if (IS_SLOT_EMPTY(slot)) {
        continue;
      }

      fn(slot);
    }
  }
}

void RecordIDSet::grow() {
  size_t new_nslots = nslots_ == 0 ? kInitialSlots : nslots_ * kGrowthFactor;

  Buffer new_data(new_nslots * SHA1Hash::kSize);
  memset(new_data.data(), 0, new_data.size());

  if (nslots_ > 0) {
    scan([this, &new_nslots, &new_data] (void* old_slot) {
      FNV<uint64_t> fnv;
      auto h = fnv.hash(old_slot, SHA1Hash::kSize);

      for (size_t i = 0; i < new_nslots; ++i) {
        size_t idx = (h + i) % new_nslots;
        auto slot = (char *) new_data.data() + idx * SHA1Hash::kSize;

        if (IS_SLOT_EMPTY(slot)) {
          memcpy(slot, old_slot, SHA1Hash::kSize);
          break;
        }
      }
    });
  }

  {
    auto file = File::openFile(
        fpath_ + "~",
        File::O_CREATEOROPEN | File::O_WRITE | File::O_TRUNCATE);

    FileHeader hdr;
    memset(&hdr, 0, sizeof(hdr));

    hdr.version = 0x1;
    hdr.nslots = new_nslots;

    file.write(&hdr, sizeof(hdr));
    file.write(new_data.data(), new_data.size());
  }

  FileUtil::mv(fpath_ + "~", fpath_);
  nslots_ = new_nslots;
}

void RecordIDSet::reopenFile() {
  auto file = File::openFile(fpath_, File::O_READ);

  FileHeader hdr;
  if (file.read(&hdr, sizeof(hdr)) != sizeof(hdr)) {
    RAISE(kRuntimeError, "error while reading file header");
  }

  if (hdr.version != 0x1) {
    RAISEF(kRuntimeError, "invalid version $0", hdr.version);
  }

  auto file_size = file.size();
  if (file_size != sizeof(FileHeader) + SHA1Hash::kSize * hdr.nslots) {
    RAISE(kRuntimeError, "invalid file size");
  }

  nslots_ = hdr.nslots;
}

void RecordIDSet::countUsedSlots() {
  nslots_used_ = 0;

  if (nslots_ > 0) {
    scan([this] (void* slot) {++nslots_used_; });
  }
}

void RecordIDSet::withMmap(
    bool readonly,
    Function<void (void* ptr)> fn) {
  auto file = File::openFile(
      fpath_,
      readonly ? File::O_READ : File::O_READ | File::O_WRITE);

  auto file_size = sizeof(FileHeader) + nslots_ * SHA1Hash::kSize;
  auto file_mmap = mmap(
      nullptr,
      file_size,
      readonly ? PROT_READ : PROT_WRITE | PROT_READ,
      MAP_SHARED,
      file.fd(),  0);

  if (file_mmap == MAP_FAILED) {
    RAISE_ERRNO(kIOError, "mmap() failed");
  }

  try {
    fn(file_mmap);
  } catch (...) {
    munmap(file_mmap, file_size);
    throw;
  }

  munmap(file_mmap, file_size); // FIXPAUL RAII...
}

} // namespace tdsb

