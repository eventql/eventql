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
#include "eventql/util/io/BufferedOutputStream.h"

namespace util {

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
