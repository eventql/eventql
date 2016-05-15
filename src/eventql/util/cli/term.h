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
#pragma once
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "eventql/util/stdtypes.h"
#include "eventql/util/io/TerminalOutputStream.h"

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
