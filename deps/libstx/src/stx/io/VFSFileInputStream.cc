/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/io/VFSFileInputStream.h>

namespace stx {

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

} // namespace stx
