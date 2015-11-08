/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "stx/io/TerminalOutputStream.h"

namespace stx {

ScopedPtr<TerminalOutputStream> TerminalOutputStream::fromStream(
    ScopedPtr<OutputStream> stream) {
  return mkScoped(new TerminalOutputStream(std::move(stream)));
}

TerminalOutputStream::TerminalOutputStream(
    ScopedPtr<OutputStream> stream) :
    os_(std::move(stream)),
    is_tty_(os_->isTTY()) {}

void TerminalOutputStream::print(
    const String& str,
    Vector<TerminalStyle> style /* = {} */) {
  if (is_tty_ && style.size() > 0) {
    String styled_str = "\e[";

    for (size_t i = 0; i < style.size(); ++i) {
      if (i > 0) styled_str += ";";
      styled_str += StringUtil::toString((uint8_t) style[i]);
    }

    styled_str += "m";
    styled_str += str;
    styled_str += "\e[0m";

    os_->write(styled_str.data(), styled_str.size());
  } else {
    os_->write(str.data(), str.size());
  }
}

size_t TerminalOutputStream::write(const char* data, size_t size) {
  return os_->write(data, size);
}

bool TerminalOutputStream::isTTY() const {
  return is_tty_;
}

}
