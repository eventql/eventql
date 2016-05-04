/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/exception.h"
#include "stx/stringutil.h"

namespace stx {
namespace json {

template <typename T>
T JSONDocument::getAs(const JSONPointer& path) const {
  T value;

  if (!getMaybeAs<T>(path, &value)) {
    RAISEF(kIndexError, "no such element: $0", path.toString());
  }

  return value;
}

template <typename T>
T JSONDocument::getAs(const JSONPointer& path, const T& fallback) const {
  T value;

  if (getMaybeAs<T>(path, &value)) {
    return value;
  } else {
    return fallback;
  }
}

} // namespace json
} // namespace stx
