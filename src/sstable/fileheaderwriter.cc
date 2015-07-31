/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/fnv.h>
#include <sstable/binaryformat.h>
#include <sstable/fileheaderwriter.h>

namespace stx {
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
    stx::util::BinaryMessageWriter(buf, buf_size) {
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
}

// DEPRECATED
FileHeaderWriter::FileHeaderWriter(
    void* buf,
    size_t buf_size) :
    stx::util::BinaryMessageWriter(buf, buf_size) {}

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
}

