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
#include <eventql/util/io/VFSFileInputStream.h>

VFSFileInputStream::VFSFileInputStream(
    RefPtr<VFSFile> data) :
    data_(data),
    cur_(0) {}

bool VFSFileInputStream::readNextByte(char* target) {
  if (cur_ < data_->size()) {
    *target = *(((char*) data_->data()) + cur_++);
    return true;
  } else {
    return false;
  }
}

size_t VFSFileInputStream::skipNextBytes(size_t n_bytes) {
  auto nskipped = n_bytes;
  if (cur_ + n_bytes > data_->size()) {
    nskipped = data_->size() - cur_;
  }

  cur_ += nskipped;
  return nskipped;
}

bool VFSFileInputStream::eof() {
  return cur_ >= data_->size();
}

void VFSFileInputStream::rewind() {
  cur_ = 0;
}

void VFSFileInputStream::seekTo(size_t offset) {
  if (offset < data_->size()) {
    cur_ = offset;
  } else {
    cur_ = data_->size();
  }
}
