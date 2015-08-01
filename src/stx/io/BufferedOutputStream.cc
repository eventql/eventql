/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/io/BufferedOutputStream.h"

namespace stx {

ScopedPtr<BufferedOutputStream> BufferedOutputStream::fromStream(
    ScopedPtr<OutputStream> stream,
    size_t buffer_size /* = kDefaultBufferSize */) {
  return mkScoped(new BufferedOutputStream(std::move(stream), buffer_size));
}

BufferedOutputStream::BufferedOutputStream(
    ScopedPtr<OutputStream> stream,
    size_t buffer_size /* = kDefaultBufferSize */) :
    os_(std::move(stream)) {
  buf_.reserve(buffer_size);
}

BufferedOutputStream::~BufferedOutputStream() {
  flush();
}

size_t BufferedOutputStream::write(const char* data, size_t size) {
  if (size <= buf_.remaining()) {
    buf_.append(data, size);
    return size;
  }

  size_t consumed = 0;
  if (buf_.remaining() > 0) {
    consumed = buf_.remaining();
    buf_.append(data, consumed);
  }

  flush();

  while (size - consumed > buf_.capacity()) {
    os_->write(data + consumed, buf_.capacity());
    consumed += buf_.capacity();
  }

  if (consumed != size) {
    buf_.append(data + consumed, size - consumed);
  }

  return size;
}

void BufferedOutputStream::flush() {
  if (buf_.size() == 0) {
    return;
  }

  os_->write((char*) buf_.data(), buf_.size());
  buf_.clear();
}


}
