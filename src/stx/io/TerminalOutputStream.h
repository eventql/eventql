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
#include "stx/io/outputstream.h"

namespace stx {

enum class TerminalStyle : uint8_t {
  BRIGHT      = 1,
  DIM         = 2,
  UNDERSCORE  = 4,
  BLINK       = 5,
  REVERSE     = 7,
  HIDDEN      = 8,
  BLACK       = 30,
  RED         = 31,
  GREEN       = 32,
  YELLOW      = 33,
  BLUE        = 34,
  MAGENTA     = 35,
  CYAN        = 36,
  WHITE       = 37,
  BG_BLACK    = 40,
  BG_RED      = 41,
  BG_GREEN    = 42,
  BG_YELLOW   = 43,
  BG_BLUE     = 44,
  BG_MAGENTA  = 45,
  BG_CYAN     = 46,
  BG_WHITE    = 47
};


/**
 * A terminal output stream allows to conditionally use common ANSI terminal
 * escape codes, to e.g. color the output or control the cursor.
 *
 * A TerminalOutputStream detects if the underlying output stream is a TTY and
 * if not, doesn't omit any escape sequences.
 */
class TerminalOutputStream : public OutputStream {
public:

  /**
   * Create a new TerminalOutputStream instance from the provided output stream
   *
   * @param stream the stream
   */
  static ScopedPtr<TerminalOutputStream> fromStream(
      ScopedPtr<OutputStream> stream);

  /**
   * Create a new TerminalOutputStream instance from the provided output stream
   *
   * @param stream the stream
   */
  TerminalOutputStream(
      ScopedPtr<OutputStream> stream);

  /**
   * Write the provided string to the stream with the specified style(s).
   * This method may raise an exception.
   *
   * @param str a string to be written/printed
   * @param style one or more TerminalStyles
   */
  void print(const String& str, Vector<TerminalStyle> style = {});

  /**
   * Write the next n bytes to the stream. This may raise an exception.
   * Returns the number of bytes that have been written.
   *
   * @param data a pointer to the data to be written
   * @param size then number of bytes to be written
   */
  size_t write(const char* data, size_t size) override;

  /**
   * Returns true if this file descriptor is connected to a TTY/terminal and
   * false otherwise
   */
  bool isTTY() const override;

protected:
  ScopedPtr<OutputStream> os_;
  bool is_tty_;
};

}
