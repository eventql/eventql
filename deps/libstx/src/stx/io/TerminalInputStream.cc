/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/io/TerminalInputStream.h"

namespace stx {

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

}
