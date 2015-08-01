/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <string.h>
#include <unistd.h>
#include <stx/util/PersistentHashSet.h>
#include <stx/io/fileutil.h>
#include <stx/io/mmappedfile.h>
#include <stx/fnv.h>

namespace stx {

const size_t PersistentHashSet::kVersion = 1;
const double PersistentHashSet::kMaxFillFactor = 0.5f;
const double PersistentHashSet::kGrowthFactor = 2.0f;
const size_t PersistentHashSet::kInitialSlots = 512;
const size_t PersistentHashSet::kFetchIOBatchSize = 200;
const size_t PersistentHashSet::kProbeIOBatchSize = 25;

#define IS_SLOT_EMPTY(slot) (\
    *((uint64_t*) (slot)) == 0 && \
    memcmp( \
        (slot), \
        (slot) + sizeof(uint64_t), \
        SHA1Hash::kSize - sizeof(uint64_t)) == 0)

PersistentHashSet::PersistentHashSet(
    const String& filepath) :
    fpath_(filepath),
    nslots_(0),
    nslots_used_(-1) {
  if (FileUtil::exists(filepath)) {
    auto file = File::openFile(fpath_, File::O_READ);

    FileHeader hdr;
    if (file.read(&hdr, sizeof(hdr)) != sizeof(hdr)) {
      RAISE(kRuntimeError, "error while reading file header");
    }

    if (hdr.version != kVersion) {
      RAISEF(kRuntimeError, "invalid version $0", hdr.version);
    }

    auto file_size = file.size();
    if (file_size != sizeof(FileHeader) + SHA1Hash::kSize * hdr.nslots) {
      RAISE(kRuntimeError, "invalid file size");
    }

    nslots_ = hdr.nslots;
  }
}

bool PersistentHashSet::addRecordID(const SHA1Hash& record_id) {
  std::unique_lock<std::mutex> lk(write_mutex_);
  auto file = File::openFile(
      fpath_,
      File::O_WRITE | File::O_READ | File::O_CREATEOROPEN);

  return insert(&file, record_id);
}

void PersistentHashSet::addRecordIDs(Set<SHA1Hash>* record_ids) {
  std::unique_lock<std::mutex> lk(write_mutex_);
  auto file = File::openFile(
      fpath_,
      File::O_WRITE | File::O_READ | File::O_CREATEOROPEN);

  for (auto cur = record_ids->begin(); cur != record_ids->end(); ) {
    if (insert(&file, *cur)) {
      ++cur;
    } else {
      cur = record_ids->erase(cur);
    }
  }
}

bool PersistentHashSet::insert(File* file, const SHA1Hash& record_id) {
  if (nslots_used_ == size_t(-1)) {
    nslots_used_ = 0;
    if (nslots_ > 0) {
      scan(file, nslots_, [this] (void* slot) {++nslots_used_; });
    }
  }

  if (nslots_used_ + 1 > nslots_ * kMaxFillFactor) {
    grow(file);
  }

  size_t insert_idx = 0;
  if (lookup(file, nslots_, record_id, &insert_idx)) {
    return false;
  } else {
    file->seekTo(sizeof(FileHeader) + insert_idx * SHA1Hash::kSize);
    file->write(record_id.data(), SHA1Hash::kSize);
    ++nslots_used_;
    return true;
  }
}

bool PersistentHashSet::hasRecordID(const SHA1Hash& record_id) {
  std::unique_lock<std::mutex> lk(read_mutex_);
  auto nslots = nslots_;
  if (nslots == 0) {
    return false;
  }

  auto file = File::openFile(fpath_, File::O_READ);
  lk.unlock();

  return lookup(&file, nslots, record_id);
}

Set<SHA1Hash> PersistentHashSet::fetchRecordIDs() {
  Set<SHA1Hash> ids;

  std::unique_lock<std::mutex> lk(read_mutex_);
  auto nslots = nslots_;
  if (nslots_ == 0) {
    return ids;
  }

  auto file = File::openFile(fpath_, File::O_READ);
  lk.unlock();

  scan(&file, nslots, [&ids] (void* slot) {
    ids.emplace(SHA1Hash(slot, SHA1Hash::kSize));
  });

  return ids;
}

// preconditions: no locks required
bool PersistentHashSet::lookup(
    File* file,
    size_t nslots,
    const SHA1Hash& record_id,
    size_t* insert_idx /* = nullptr */) {
  FNV<uint64_t> fnv;
  auto h = fnv.hash(record_id.data(), record_id.size());

  Buffer buf(kProbeIOBatchSize * SHA1Hash::kSize);
  size_t buf_offset = -1;
  size_t buf_size = 0;

  for (size_t i = 0; i < nslots; ++i) {
    auto idx = (h + i) % nslots;

    if (!buf_size || idx >= buf_offset + buf_size || idx < buf_offset) {
      file->seekTo(sizeof(FileHeader) + idx * SHA1Hash::kSize);
      auto nread = file->read(buf.data(), buf.size());
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

// preconditions: no locks required
void PersistentHashSet::scan(
    File* file,
    size_t nslots,
    Function<void (void* slot)> fn) {
  file->seekTo(sizeof(FileHeader));

  Buffer buf(kFetchIOBatchSize * SHA1Hash::kSize);
  auto nbatches = (nslots + kFetchIOBatchSize - 1) / kFetchIOBatchSize;
  for (size_t b = 0; b < nbatches; ++b) {
    auto nread = file->read(buf.data(), buf.size());
    if (nread == -1) {
      break;
    }

    for (size_t pos = 0; pos < SHA1Hash::kSize * kFetchIOBatchSize;
        pos += SHA1Hash::kSize) {
      auto slot = static_cast<char*>(buf.data()) + pos;
      if (IS_SLOT_EMPTY(slot)) {
        continue;
      }

      fn(slot);
    }
  }
}

// preconditions: must hold write lock
void PersistentHashSet::grow(File* file) {
  size_t new_nslots = nslots_ == 0 ? kInitialSlots : nslots_ * kGrowthFactor;

  Buffer new_data(new_nslots * SHA1Hash::kSize);
  memset(new_data.data(), 0, new_data.size());

  if (nslots_ > 0) {
    scan(file, nslots_, [this, &new_nslots, &new_data] (void* old_slot) {
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
    auto new_file = File::openFile(
        fpath_ + "~",
        File::O_CREATEOROPEN | File::O_WRITE | File::O_READ | File::O_TRUNCATE);

    FileHeader hdr;
    memset(&hdr, 0, sizeof(hdr));

    hdr.version = kVersion;
    hdr.nslots = new_nslots;

    new_file.write(&hdr, sizeof(hdr));
    new_file.write(new_data.data(), new_data.size());
    *file = std::move(new_file);
  }

  std::unique_lock<std::mutex> lk(read_mutex_);
  FileUtil::mv(fpath_ + "~", fpath_);
  nslots_ = new_nslots;
}

} // namespace stx

