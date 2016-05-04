/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-logtable/TableChunkSummaryReader.h>
#include <stx/util/binarymessagereader.h>

namespace stx {
namespace logtable {

TableChunkSummaryReader::TableChunkSummaryReader(
    const String& filename) :
    mmap_(File::openFile(filename, File::O_READ)),
    body_offset_(0) {
  util::BinaryMessageReader reader(mmap_.data(), mmap_.size());
  auto n = *reader.readUInt32();

  for (int i = 0; i < n; ++i) {
    auto name_len = reader.readVarUInt();
    String name((char *) reader.read(name_len), name_len);
    auto offset = reader.readVarUInt();
    auto size = reader.readVarUInt();
    offsets_.emplace(name, std::make_pair(offset, size));
  }

  body_offset_ = reader.position();
}

Option<Buffer> TableChunkSummaryReader::getSummaryData(
    const String& summary_name) const {
  void* data;
  size_t size;

  if (getSummaryData(summary_name, &data, &size)) {
    return Some(Buffer(data, size));
  } else {
    return None<Buffer>();
  }
}

bool TableChunkSummaryReader::getSummaryData(
    const String& summary_name,
    void** data,
    size_t* size) const {
  auto s = offsets_.find(summary_name);
  if (s == offsets_.end()) {
    return false;
  }

  *data = mmap_.structAt<void>(s->second.first + body_offset_);
  *size = s->second.second;
  return true;
}


}
}
