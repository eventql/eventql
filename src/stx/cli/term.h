/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "stx/stdtypes.h"
#include "stx/io/TerminalOutputStream.h"

namespace stx {

class Term {
public:

  Term();
  ~Term();

  /**
   * Reads a "yes/no" response from STDIN. Accepted responses:
   *   - Y/y -> true
   *   - N/n -> false
   *   - everything else -> RAISE exception
   */
  bool readConfirmation();

  char readChar();

  String readLine(
      const String& prompt = "");

  String readPassword();

  /**
   * Write the provided string to the stream with the specified style(s).
   * This method may raise an exception.
   *
   * @param str a string to be written/printed
   * @param style one or more TerminalStyles
   */
  void print(const String& str, Vector<TerminalStyle> style = {});

  /**
   * Write the provided string to the stream with in red color
   *
   * @param str a string to be written/printed
   */
  void printRed(const String& str);

  /**
   * Write the provided string to the stream with in green color
   *
   * @param str a string to be written/printed
   */
  void printGreen(const String& str);

  /**
   * Write the provided string to the stream with in yellow color
   *
   * @param str a string to be written/printed
   */
  void printYellow(const String& str);

  /**
   * Write the provided string to the stream with in blue color
   *
   * @param str a string to be written/printed
   */
  void printBlue(const String& str);

  /**
   * Write the provided string to the stream with in magenta color
   *
   * @param str a string to be written/printed
   */
  void printMagenta(const String& str);

  /**
   * Write the provided string to the stream with in cyan color
   *
   * @param str a string to be written/printed
   */
  void printCyan(const String& str);

  /**
   * Erases from the current cursor position to the end of the current line.
   */
  void eraseEndOfLine();

  /**
   * Erases from the current cursor position to the start of the current line.
   */
  void eraseStartOfLine();

  /**
   * Erases the entire current line.
   */
  void eraseLine();

  /**
   * Erases the screen from the current line down to the bottom of the screen.
   */
  void eraseDown();

  /**
   * Erases the screen from the current line up to the top of the screen.
   */
  void eraseUp();

  /**
   * Erases the screen with the background colour and moves the cursor to home.
   */
  void eraseScreen();

  /**
   * Text wraps to next line if longer than the length of the display area.
   */
  void enableLineWrap();

  /**
   * Disables line wrapping.
   */
  void disableLineWrap();

  /**
   * Set the title of this terminal if supported
   */
  void setTitle(const String& title);

protected:

  bool enableRawMode();
  void disableRawMode();

  ScopedPtr<TerminalOutputStream> termos_;
  struct termios orig_termios_;
  bool rawmode_;
};

} // namespace stx
