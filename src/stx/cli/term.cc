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
#include <termios.h>
#include <unistd.h>

namespace stx {

Term::Term() :
    termos_(TerminalOutputStream::fromStream(OutputStream::getStdout())) {}

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

String Term::readLine(const String& prompt) {
  String line;

  if (!prompt.empty())  {
    printf("%s", prompt.c_str());
    fflush(stdout);
  }

  for (;;) {
    char chr = readChar();
    if (chr == '\r') continue;
    if (chr == '\n') break;
    line += chr;
  }

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

} // namespace stx
