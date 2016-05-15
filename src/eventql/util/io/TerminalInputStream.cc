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
#include "eventql/util/io/TerminalInputStream.h"

ScopedPtr<TerminalInputStream> TerminalInputStream::fromStream(
    ScopedPtr<FileInputStream> stream) {
  return mkScoped(new TerminalInputStream(std::move(stream)));
}

TerminalInputStream::TerminalInputStream(
    ScopedPtr<FileInputStream> stream) :
    is_(std::move(stream)) {}

bool TerminalInputStream::readNextByte(char* target) {
  return is_->readNextByte(target);
}

size_t TerminalInputStream::readNextBytes(void* target, size_t n_bytes) {
  return is_->readNextBytes(target, n_bytes);
}

size_t TerminalInputStream::skipNextBytes(size_t n_bytes) {
  return is_->skipNextBytes(n_bytes);
}

bool TerminalInputStream::eof() {
  return is_->eof();
}
