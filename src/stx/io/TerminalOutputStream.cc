/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
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

void TerminalOutputStream::printRed(const String& str) {
  print(str, { TerminalStyle::RED });
}

void TerminalOutputStream::printGreen(const String& str) {
  print(str, { TerminalStyle::GREEN });
}

void TerminalOutputStream::printYellow(const String& str) {
  print(str, { TerminalStyle::YELLOW });
}

void TerminalOutputStream::printBlue(const String& str) {
  print(str, { TerminalStyle::BLUE });
}

void TerminalOutputStream::printMagenta(const String& str) {
  print(str, { TerminalStyle::MAGENTA });
}

void TerminalOutputStream::printCyan(const String& str) {
  print(str, { TerminalStyle::CYAN });
}

void TerminalOutputStream::eraseEndOfLine() {
  static const char kEscapeSequence[] = "\e[K";
  os_->write(kEscapeSequence, sizeof(kEscapeSequence) - 1);
}

void TerminalOutputStream::eraseStartOfLine() {
  static const char kEscapeSequence[] = "\e[1K";
  os_->write(kEscapeSequence, sizeof(kEscapeSequence) - 1);
}

void TerminalOutputStream::eraseLine() {
  static const char kEscapeSequence[] = "\e[2K";
  os_->write(kEscapeSequence, sizeof(kEscapeSequence) - 1);
}

void TerminalOutputStream::eraseDown() {
  static const char kEscapeSequence[] = "\e[J";
  os_->write(kEscapeSequence, sizeof(kEscapeSequence) - 1);
}

void TerminalOutputStream::eraseUp() {
  static const char kEscapeSequence[] = "\e[1J";
  os_->write(kEscapeSequence, sizeof(kEscapeSequence) - 1);
}

void TerminalOutputStream::eraseScreen() {
  static const char kEscapeSequence[] = "\e[2J";
  os_->write(kEscapeSequence, sizeof(kEscapeSequence) - 1);
}

void TerminalOutputStream::enableLineWrap() {
  static const char kEscapeSequence[] = "\e[7h";
  os_->write(kEscapeSequence, sizeof(kEscapeSequence) - 1);
}

void TerminalOutputStream::disableLineWrap() {
  static const char kEscapeSequence[] = "\e[7l";
  os_->write(kEscapeSequence, sizeof(kEscapeSequence) - 1);
}

size_t TerminalOutputStream::write(const char* data, size_t size) {
  return os_->write(data, size);
}

bool TerminalOutputStream::isTTY() const {
  return is_tty_;
}

void TerminalOutputStream::setTitle(const String& title) {
  print(StringUtil::format("\033]0;$0\007", title));
}

}
