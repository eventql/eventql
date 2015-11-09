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

void HTTPSSEParser::onEvent(Function<void (const HTTPSSEEvent& ev)> fn) {
  on_event_ = fn;
}

void HTTPSSEParser::parse(const char* data, size_t size) {
  buf_.append(data, size);

  auto begin = 0;
  auto end = buf_.size();
  size_t cur = 0;
  while (begin < end) {
    size_t state = 0;
    for (; state < 2 && cur < end; ++cur) {
      switch (*buf_.structAt<char>(cur)) {

        case '\n':
          ++state;
          break;

        case '\r':
          ++state;
          if (cur + 1 < end && *buf_.structAt<char>(cur + 1) == '\n') {
            ++cur;
          }
          break;

        default:
          state = 0;
          break;

      }
    }

    if (state == 2) {
      parseEvent(buf_.structAt<char>(begin), cur - begin);
      begin = cur;
    } else {
      break;
    }
  }

  if (begin > 0) {
    auto new_size = buf_.size() - begin;
    memmove(buf_.data(), (char*) buf_.data() + begin, new_size);
    buf_.resize(new_size);
  }
}

void HTTPSSEParser::parseEvent(const char* data, size_t size) {
  static const String kDataFieldPrefix = "data: ";
  static const String kNameFieldPrefix = "event: ";

  HTTPSSEEvent event;

  auto begin = 0;
  for (size_t cur = 0; cur < size; ++cur) {
    switch (data[cur]) {

      case '\r':
      case '\n':
        if (cur > begin) {
          String line(data + begin, cur - begin);

          if (StringUtil::beginsWith(line, kDataFieldPrefix)) {
            event.data = line.substr(kDataFieldPrefix.size());
          }

          else if (StringUtil::beginsWith(line, kNameFieldPrefix)) {
            event.name = Some(line.substr(kNameFieldPrefix.size()));
          }
        }

        if (cur + 1 < size && data[cur] == '\r' && data[cur + 1] == '\n') {
          ++cur;
        }

        begin = cur + 1;
        break;

    }
  }

  if (!event.data.empty() && on_event_) {
    on_event_(event);
  }
}

}
}
