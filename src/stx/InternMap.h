/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_BASE_INTERNAMP_H
#define _STX_BASE_INTERNAMP_H
#include <mutex>
#include "stx/stdtypes.h"

namespace stx {

class InternMap {
public:
  static const constexpr uint32_t kMagic = 0x17234205;

  void* internString(const String& str);
  String getString(const void* interned);

protected:
  struct InternedStringHeader {
    uint32_t magic;
    size_t size;
  };

  std::mutex intern_map_mutex_;
  HashMap<String, void*> intern_map_;
};

} // namespace stx
#endif
