/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/io/sstable/binaryformat.h>
#include <eventql/io/sstable/fileheaderreader.h>
#include <eventql/util/exception.h>
#include <eventql/util/fnv.h>

namespace sstable {

MetaPage FileHeaderReader::readMetaPage(InputStream* is) {
  MetaPage hdr;

  auto magic_bytes = is->readUInt32();
  if (magic_bytes != BinaryFormat::kMagicBytes) {
    RAISE(kIllegalStateError, "not a valid sstable");
  }

  hdr.version_ = is->readUInt16();

  switch (hdr.version_) {

    case 0x1:
      hdr.flags_ = 0;
      hdr.num_rows_ = uint64_t(-1);
      break;

    case 0x2:
      hdr.flags_ = is->readUInt64();
      hdr.num_rows_ = uint64_t(-1);
      break;

    case 0x3:
      hdr.flags_ = is->readUInt64();
      hdr.num_rows_ = is->readUInt64();
      break;

    default:
      RAISE(kIllegalStateError, "unsupported sstable version");

  }

  hdr.body_size_ = is->readUInt64();
  hdr.userdata_checksum_ = is->readUInt32();
  hdr.userdata_size_ = is->readUInt32();

  /* pre version 0x02 body_size > 0 implied that the table is finalized */
  if (hdr.version_ == 0x01 && hdr.body_size_ > 0) {
    hdr.flags_ |= (uint64_t) FileHeaderFlags::FINALIZED;
  }

  return hdr;
}

FileHeaderReader::FileHeaderReader(
    void* buf,
    size_t buf_size) :
    util::BinaryMessageReader(buf, buf_size),
    file_size_(buf_size) {
  MemoryInputStream is(buf, buf_size);
  hdr_ = FileHeaderReader::readMetaPage(&is);
}

bool FileHeaderReader::verify() {
  if (hdr_.headerSize() > file_size_) {
    return false;
  }

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
  seekTo(hdr_.userdataOffset());
  *userdata = read(*userdata_size);
}

}

