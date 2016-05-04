/*
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/UTF8.h"
#include "eventql/util/SimpleTokenizer.h"

namespace stx {
namespace fts {

void SimpleTokenizer::tokenize(
    const stx::String& query,
    Function<void (const stx::String& term)> term_callback) const {
  stx::String buf;

  auto cur = query.c_str();
  auto end = cur + query.size() + 1;
  while (cur < end) {
    auto chr = UTF8::nextCodepoint(&cur, end);

    // skip all symbols/dingbats characters
    if (chr > 0x2000 && chr < 0x2BFF) {
      continue;
    }

    switch (chr) {

      /* token boundaries */
      case '!':
      case '"':
      case '#':
      case '$':
      case '%':
      case '&':
      case '\'':
      case '(':
      case ')':
      case '*':
      case '+':
      case ',':
      case '.':
      case '-':
      case '/':
      case ':':
      case ';':
      case '<':
      case '=':
      case '>':
      case '?':
      case '@':
      case '[':
      case '\\':
      case ']':
      case '^':
      case '_':
      case '`':
      case '{':
      case '|':
      case '}':
      case '~':
      case ' ':
      case '\t':
      case '\r':
      case '\n':
      case 0:
        break;

      /* valid chars */
      default:
        UTF8::encodeCodepoint(std::tolower(chr), &buf);
        continue;

    }

    if (buf.length() > 0) {
      term_callback(buf);
      buf.clear();
    }
  }
}

} // namespace fts
} // namespace stx

