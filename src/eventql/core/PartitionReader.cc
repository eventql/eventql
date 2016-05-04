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
#include <eventql/infra/sstable/sstablereader.h>
#include <eventql/core/PartitionReader.h>

using namespace stx;

namespace zbase {

PartitionReader::PartitionReader(
    RefPtr<PartitionSnapshot> head) :
    snap_(head) {}

Option<RefPtr<VFSFile>> PartitionReader::fetchCSTable() const {
  auto filepath = fetchCSTableFilename();

  if (filepath.isEmpty()) {
    return None<RefPtr<VFSFile>>();
  } else {
    return Some<RefPtr<VFSFile>>(
        new io::MmappedFile(File::openFile(filepath.get(), File::O_READ)));
  }
}

Option<String> PartitionReader::fetchCSTableFilename() const {
  auto filepath = FileUtil::joinPaths(snap_->base_path, "_cstable");

  if (FileUtil::exists(filepath)) {
    return Some(filepath);
  } else {
    return None<String>();
  }
}

Option<String> PartitionReader::cstableFilename() const {
  auto filepath = FileUtil::joinPaths(snap_->base_path, "_cstable");

  if (FileUtil::exists(filepath)) {
    return Some(filepath);
  } else {
    return None<String>();
  }
}

Option<SHA1Hash> PartitionReader::cstableVersion() const {
  auto metapath = FileUtil::joinPaths(snap_->base_path, "_cstable_state");

  if (FileUtil::exists(metapath)) {
    return Some(SHA1::compute(FileUtil::read(metapath)));
  }

  if (snap_->state.has_cstable_version()) {
    return Some(
        SHA1::compute(StringUtil::toString(snap_->state.cstable_version())));
  }

  return None<SHA1Hash>();
}

} // namespace tdsb

