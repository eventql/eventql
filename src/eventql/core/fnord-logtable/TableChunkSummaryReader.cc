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
#include <fnord-logtable/TableChunkSummaryReader.h>
#include <eventql/util/util/binarymessagereader.h>

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
