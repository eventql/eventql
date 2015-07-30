/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <sstable/binaryformat.h>
#include <sstable/fileheaderreader.h>
#include <stx/exception.h>
#include <stx/fnv.h>

namespace stx {
namespace sstable {

FileHeaderReader::FileHeaderReader(
    void* buf,
    size_t buf_size) :
    file_size_(buf_size) {
  MemoryInputStream is(buf, buf_size);
  hdr_ = FileHeader::readMetaPage(&is);
}

bool FileHeaderReader::verify() {
  if (hdr_.userdataOffset() + hdr_.userdataSize() > file_size_) {
    return false;
  }

  if (hdr_.userdataSize() == 0) {
    return true;
  }

  const void* userdata;
  size_t userdata_size;
  readUserdata(&userdata, &userdata_size);

  FNV<uint32_t> fnv;
  uint32_t userdata_checksum = fnv.hash(userdata, userdata_size);

  return userdata_checksum == hdr_.userdataChecksum();
}

size_t FileHeaderReader::headerSize() const {
  return hdr_.headerSize();
}

size_t FileHeaderReader::bodySize() const {
  return hdr_.bodySize();
}

bool FileHeaderReader::isFinalized() const {
  return hdr_.isFinalized();
}

size_t FileHeaderReader::userdataSize() const {
  return hdr_.userdataSize();
}

void FileHeaderReader::readUserdata(
    const void** userdata,
    size_t* userdata_size) {
  *userdata_size = hdr_.userdataSize();
  //seekTo(hdr.userdataOffset());
  //*userdata = read(*userdata_size);
  // : public stx::util::BinaryMessageReader 
}

}
}

