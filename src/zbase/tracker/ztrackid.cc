/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/exception.h>
#include <stx/inspect.h>
#include <zbase/tracker/ztrackid.h>

using namespace stx;

namespace zbase {

String ztrackid_decode(const String& encoded) {
  static const char base36[37] = "0123456789abcdefghijklmnopqrstuvwxyz";

  auto decoded = (char*) alloca(encoded.size());
  if (!decoded) {
    RAISE(kMallocError, "alloca() failed");
  }

  auto begin = encoded.data();
  auto len = encoded.size();
  auto end = begin + len;
  auto out = decoded + len;
  char lastchr = 0;
  for (auto cur = begin + len - 1 - (len - 1) % 13; cur >= begin;
      end = cur, cur -= 13) {
    uint64_t v = 0;

    for (auto chr = cur; chr < end; ++chr) {
      auto c = *chr;
      if (c >= '0' && c <= '9') {
        c -= '0';
      } else if (c >= 'A' && c <= 'X') {
        c -= 'A' - 10;
      } else if (c >= 'a' && c <= 'x') {
        c -= 'a' - 10;
      } else {
        break;
      }

      v *= 34;
      v += c;
    }

    do {
      auto chr = base36[v % 36];

      if (chr == 'x') {
        switch (lastchr) {

          case 'a':
            *out = '_';
            continue;

          case 'b':
            *out = '-';
            continue;

          case 'x':
          case '0':
            continue;

        }
      }

      *(--out) = lastchr = chr;
    } while (v /= 36);
  }

  return String(out, encoded.size() - (out - decoded));
}

} // namespace zbase

