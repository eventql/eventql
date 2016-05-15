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
#include "eventql/util/io/TerminalOutputStream.h"

namespace util {

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
