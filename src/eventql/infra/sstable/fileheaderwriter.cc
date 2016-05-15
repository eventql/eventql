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
#include <eventql/infra/sstable/binaryformat.h>
#include <eventql/infra/sstable/fileheaderwriter.h>


namespace sstable {

size_t FileHeaderWriter::calculateSize(size_t userdata_size) {
  return 30 + userdata_size; // FIXPAUL
}

void FileHeaderWriter::writeMetaPage(
    const MetaPage& header,
    OutputStream* os) {
  if (header.version() != BinaryFormat::kVersion) {
    RAISE(kIllegalStateError, "unsupported sstable version");
  }

  os->appendUInt32(BinaryFormat::kMagicBytes);
  os->appendUInt16(header.version());
  os->appendUInt64(header.flags());
  os->appendUInt64(header.rowCount());
  os->appendUInt64(header.bodySize());
  os->appendUInt32(header.userdataChecksum());
  os->appendUInt32(header.userdataSize());
}

void FileHeaderWriter::writeHeader(
    const MetaPage& header,
    const void* userdata,
    size_t userdata_size,
    OutputStream* os) {
  if (userdata_size != header.userdataSize()) {
    RAISE(kIllegalStateError, "userdata size does not match meta page");
  }

  writeMetaPage(header, os);
  os->write((char*) userdata, userdata_size);
}

// DEPRECATED
FileHeaderWriter::FileHeaderWriter(
    void* buf,
    size_t buf_size,
    size_t body_size,
    const void* userdata,
    size_t userdata_size) :
    util::BinaryMessageWriter(buf, buf_size) {
  uint64_t flags = 0;

  appendUInt32(BinaryFormat::kMagicBytes);
  appendUInt16(0x2);
  appendUInt64(flags);
  appendUInt64(body_size);

  if (userdata_size > 0) {
    FNV<uint32_t> fnv;
    auto userdata_checksum = fnv.hash(userdata, userdata_size);
    appendUInt32(userdata_checksum);
    appendUInt32(userdata_size);
    append(userdata, userdata_size);
  } else {
    appendUInt32(0);
    appendUInt32(0);
  }

// DEPRECATED
FileHeaderWriter::FileHeaderWriter(
    void* buf,
    size_t buf_size) :
    util::BinaryMessageWriter(buf, buf_size) {}

// DEPRECATED
void FileHeaderWriter::updateBodySize(size_t body_size) {
  updateUInt64(14, body_size); // FIXPAUL
}

// DEPRECATED
void FileHeaderWriter::setFlag(FileHeaderFlags flag) {
  auto flags = *((uint64_t*) (((char*) ptr_) + 6));
  flags |= (uint64_t) flag;
  updateUInt64(6, flags);
}

}

