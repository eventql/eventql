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

FileHeader FileHeader::createHeader(
    const void* userdata,
    size_t userdata_size) {

  FNV<uint32_t> fnv;
  FileHeader hdr;

  hdr.version_ = 2;
  hdr.userdata_size_ = userdata_size;
  hdr.userdata_checksum_ = fnv.hash(userdata, userdata_size);
  return hdr;
}

FileHeader::FileHeader() :
  version_(0),
  flags_(0),
  body_size_(0),
  userdata_checksum_(0),
  userdata_size_(0),
  userdata_offset_(0) {}

size_t FileHeader::headerSize() const {
  return bodyOffset();
}

size_t FileHeader::bodySize() const {
  return body_size_;
}

size_t FileHeader::bodyOffset() const {
  return userdata_offset_ + userdata_size_;
}

bool FileHeader::isFinalized() const {
  return (flags_ & (uint64_t) FileHeaderFlags::FINALIZED) > 0;
}

size_t FileHeader::userdataSize() const {
  return userdata_size_;
}

size_t FileHeader::userdataOffset() const {
  return userdata_offset_;
}

uint32_t FileHeader::userdataChecksum() const {
  return userdata_checksum_;
}

}
}
