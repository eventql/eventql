/**
 * This file is part of the "libsstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/fnv.h>
#include <sstable/FileHeader.h>
#include <sstable/binaryformat.h>

namespace stx {
namespace sstable {

FileHeader::FileHeader(
    const void* userdata,
    size_t userdata_size) :
    FileHeader() {
  FNV<uint32_t> fnv;
  FileHeader hdr;

  version_ = BinaryFormat::kVersion;
  userdata_size_ = userdata_size;
  userdata_checksum_ = fnv.hash(userdata, userdata_size);
}

FileHeader::FileHeader() :
  version_(0),
  flags_(0),
  body_size_(0),
  userdata_checksum_(0),
  userdata_size_(0) {}

size_t FileHeader::userdataOffset() const {
  switch (version_) {

    case 0x1:
      return 22;
      break;

    case 0x2:
      return 30;
      break;

    default:
      RAISE(kIllegalStateError, "unsupported sstable version");

  }
}

uint16_t FileHeader::version() const {
  return version_;
}

size_t FileHeader::headerSize() const {
  return bodyOffset();
}

size_t FileHeader::bodySize() const {
  return body_size_;
}

void FileHeader::setBodySize(size_t new_body_size) {
  body_size_ = new_body_size;
}

size_t FileHeader::bodyOffset() const {
  return userdataOffset() + userdata_size_;
}

bool FileHeader::isFinalized() const {
  return (flags_ & (uint64_t) FileHeaderFlags::FINALIZED) > 0;
}

size_t FileHeader::userdataSize() const {
  return userdata_size_;
}

uint32_t FileHeader::userdataChecksum() const {
  return userdata_checksum_;
}

uint64_t FileHeader::flags() const {
  return flags_;
}

}
}
