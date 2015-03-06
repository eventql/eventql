/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-base/InternMap.h"

namespace fnord {

void* InternMap::internString(const String& str) {
  std::unique_lock<std::mutex> lk(intern_map_mutex_);

  auto iter = intern_map_.find(str);
  if (iter != intern_map_.end()) {
    return iter->second;
  }

  auto istr = new String(str);
  intern_map_[str] = istr;
  return istr;
}

String InternMap::getString(const void* interned) {
  return *((String *) interned);
}

} // namespace fnord
