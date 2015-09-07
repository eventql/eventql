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

} // namespace stx
