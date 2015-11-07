/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/http/HTTPSSEParser.h"

namespace stx {
namespace http {

void HTTPSSEParser::parse(const char* data, size_t size) {
  buf.append(data, size);

  auto begin = 0;
  auto end = buf.size();
  size_t cur = 0;
  for (;;) {
    size_t state = 0;
    for (; state < 2 && cur < end; ++cur) {
      switch (*buf.structAt<char>(cur)) {

        case '\n':
          ++state;
          break;

        case '\r':
          ++state;
          if (cur + 1 < end && *buf.structAt<char>(cur) == '\n') {
            ++cur;
          }
          break;

        default:
          state = 0;
          break;

      }
    }

    if (state == 2) {
      parseEvent(buf.structAt<char>(begin), cur - begin);
      begin = cur;
    } else {
      break;
    }
  }
}

void HTTPSSEParser::parseEvent(const char* data, size_t size) {
  iputs("event: $0", String(data, size));
}

}
}
