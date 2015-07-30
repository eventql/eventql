/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/RecordIDSet.h>
#include <stx/io/fileutil.h>

using namespace stx;

namespace tsdb {

const double RecordIDSet::kMaxFillFactor = 0.5f;
const double RecordIDSet::kGrowthFactor = 2.0f;
const size_t RecordIDSet::kInitialSlots = 512;

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
}

bool RecordIDSet::hasRecordID(const SHA1Hash& record_id) {
  if (nslots_ == 0) {
    return false;
  }

  return false;
}

Set<SHA1Hash> RecordIDSet::fetchRecordIDs() {
  Set<SHA1Hash> ids;
  return ids;
}

void RecordIDSet::grow() {
    auto new_nslots = nslots_ == 0 ? kInitialSlots : nslots_ * kGrowthFactor;
    auto new_size = sizeof(FileHeader) + SHA1Hash::kSize * new_nslots;

  {
    auto file = File::openFile(
        fpath_ + "~",
        File::O_CREATEOROPEN | File::O_WRITE | File::O_TRUNCATE);

    FileHeader hdr;
    memset(&hdr, 0, sizeof(hdr));

    hdr.version = 0x1;
    hdr.nslots = new_nslots;

    file.write(&hdr, sizeof(hdr));
    file.truncate(new_size);
  }

  FileUtil::mv(fpath_ + "~", fpath_);
  nslots_ = new_nslots;
}

void RecordIDSet::reopenFile() {

}

void RecordIDSet::countUsedSlots() {
  if (nslots_ == 0) {
    nslots_used_ = 0;
    return;
  }

  RAISE(kNotYetImplementedError);
}

} // namespace tdsb

