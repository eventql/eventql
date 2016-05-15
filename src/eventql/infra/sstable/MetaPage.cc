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
#include <eventql/infra/sstable/MetaPage.h>
#include <eventql/infra/sstable/binaryformat.h>

namespace sstable {

MetaPage::MetaPage(
    const void* userdata,
    size_t userdata_size) :
    MetaPage() {
  FNV<uint32_t> fnv;
  MetaPage hdr;

  version_ = BinaryFormat::kVersion;
  userdata_size_ = userdata_size;
  userdata_checksum_ = fnv.hash(userdata, userdata_size);
}

MetaPage::MetaPage() :
  version_(0),
  flags_(0),
  body_size_(0),
  num_rows_(0),
  userdata_checksum_(0),
  userdata_size_(0) {}

// FIXPAUL move logic into fileheaderreader
size_t MetaPage::userdataOffset() const {
  switch (version_) {

    case 0x1:
      return 22;
      break;

    case 0x2:
      return 30;
      break;

    case 0x3:
      return 38;
      break;

    default:
      RAISE(kIllegalStateError, "unsupported sstable version");

  }
}

uint16_t MetaPage::version() const {
  return version_;
}

size_t MetaPage::headerSize() const {
  return bodyOffset();
}

size_t MetaPage::rowCount() const {
  return num_rows_;
}

void MetaPage::setRowCount(size_t new_row_count) {
  num_rows_ = new_row_count;
}

size_t MetaPage::bodySize() const {
  return body_size_;
}

void MetaPage::setBodySize(size_t new_body_size) {
  body_size_ = new_body_size;
}

size_t MetaPage::bodyOffset() const {
  return userdataOffset() + userdata_size_;
}

bool MetaPage::isFinalized() const {
  return (flags_ & (uint64_t) FileHeaderFlags::FINALIZED) > 0;
}

size_t MetaPage::userdataSize() const {
  return userdata_size_;
}

uint32_t MetaPage::userdataChecksum() const {
  return userdata_checksum_;
}

uint64_t MetaPage::flags() const {
  return flags_;
}

}
