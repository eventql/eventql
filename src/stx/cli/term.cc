/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/cli/term.h"
#include "stx/exception.h"
#include "stx/inspect.h"
#include <termios.h>
#include <unistd.h>

namespace stx {

Term::Term() :
    termos_(TerminalOutputStream::fromStream(OutputStream::getStdout())) {}

Term::~Term() {
  disableRawMode();
}

char Term::readChar() {
  char chr;
  if (read(STDIN_FILENO, &chr, sizeof(chr)) != 1) {
    RAISE_ERRNO(kIOError, "read(STDIN) failed");
  }

  return chr;
}

bool Term::readConfirmation() {
  struct termios old_tc, new_tc;

  char resp = '-';
  if (tcgetattr(STDIN_FILENO, &old_tc) == 0) {
    new_tc = old_tc;
    new_tc.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tc);
    resp = readChar();
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tc);
  } else {
    resp = readChar();
  }

  switch (resp) {

    case 'Y':
    case 'y':
      return true;

    case 'N':
    case 'n':
      return false;

    default:
      RAISEF(kIOError, "invalid response: $0", String(&resp, 1));

  }
}

String Term::readPassword() {
  struct termios old_tc, new_tc;

  if (tcgetattr(STDIN_FILENO, &old_tc) != 0) {
    RAISE(kIOError, "input is not a TTY");
  }

  new_tc = old_tc;
  new_tc.c_lflag &= ~(ECHO | ICANON);

  if (tcsetattr(STDIN_FILENO, TCSANOW, &new_tc) != 0) {
    RAISE(kIOError, "input is not a TTY");
  }

  auto resp = readLine();
  tcsetattr(STDIN_FILENO, TCSANOW, &old_tc);
  return resp;
}

String Term::readLine(const String& prompt) {
  String line;

  if (!prompt.empty())  {
    printf("%s", prompt.c_str());
    fflush(stdout);
  }

  //enableRawMode();

  for (;;) {
    char chr = readChar();
    if (chr == '\r') continue;
    if (chr == '\n') break;
    line += chr;
  }

  //disableRawMode();

  return line;
}

void Term::print(const String& str, Vector<TerminalStyle> style /* = {} */) {
  termos_->print(str, style);
}

void Term::printRed(const String& str) {
  termos_->printRed(str);
}

void Term::printGreen(const String& str) {
  termos_->printGreen(str);
}

void Term::printYellow(const String& str) {
  termos_->printYellow(str);
}

void Term::printBlue(const String& str) {
  termos_->printBlue(str);
}

void Term::printMagenta(const String& str) {
  termos_->printMagenta(str);
}

void Term::printCyan(const String& str) {
  termos_->printCyan(str);
}

void Term::eraseEndOfLine() {
  termos_->eraseEndOfLine();
}

void Term::eraseStartOfLine() {
  termos_->eraseStartOfLine();
}

void Term::eraseLine() {
  termos_->eraseLine();
}

void Term::eraseDown() {
  termos_->eraseDown();
}

void Term::eraseUp() {
  termos_->eraseUp();
}

void Term::eraseScreen() {
  termos_->eraseScreen();
}

void Term::enableLineWrap() {
  termos_->enableLineWrap();
}

void Term::disableLineWrap() {
  termos_->disableLineWrap();
}

/**
 * enableRawMode and disableRawMode methods based on antirez's linenoise (MIT):
 *   https://github.com/antirez/linenoise/blob/master/linenoise.c
 */
bool Term::enableRawMode() {
  if (!isatty(STDIN_FILENO)) {
    return false;
  }

  if (tcgetattr(STDIN_FILENO, &orig_termios_) == -1) {
    return false;
  }

  auto raw = orig_termios_;  /* modify the original mode */
  /* input modes: no break, no CR to NL, no parity check, no strip char,
   * no start/stop output control. */
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  /* output modes - disable post processing */
  raw.c_oflag &= ~(OPOST);
  /* control modes - set 8 bit chars */
  raw.c_cflag |= (CS8);
  /* local modes - choing off, canonical off, no extended functions,
   * no signal chars (^Z,^C) */
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  /* control chars - set return condition: min number of bytes and timer.
   * We want read to return every single byte, without timeout. */
  raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

  /* put terminal in raw mode after flushing */
  if (tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) < 0) {
    return false;
  }

  rawmode_ = 1;
  return true;
}

void Term::disableRawMode() {
  /* Don't even check the return value as it's too late. */
  if (rawmode_ && tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios_) != -1) {
    rawmode_ = 0;
  }
}

void Term::setTitle(const String& title) {
  termos_->setTitle(title);
}

} // namespace stx
