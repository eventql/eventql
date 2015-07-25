// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cortex-base/IEEE754.h>
#include <cortex-base/RuntimeError.h>
#include <limits>
#include <cstring>
#include <cstdio>

namespace cortex {

uint64_t IEEE754::toBytes(double value) {
  uint64_t bytes;

  constexpr bool ieee754 =
      std::numeric_limits<double>::is_iec559 &&
      sizeof(double) == sizeof(uint64_t);

  static_assert(
      ieee754,
      "IEEE 754 floating point conversion not yet implemented");

  if (ieee754) {
    memcpy((void *) &bytes, (void *) &value, sizeof(bytes));
  } else {
    /* not yet implemented */
  }

  return bytes;
}

double IEEE754::fromBytes(uint64_t bytes) {
  double value;

  constexpr bool ieee754 =
      std::numeric_limits<double>::is_iec559 &&
      sizeof(double) == sizeof(uint64_t);

  static_assert(
      ieee754,
      "IEEE 754 floating point conversion not yet implemented");

  if (ieee754) {
    memcpy((void *) &value, (void *) &bytes, sizeof(bytes));
  } else {
    /* not yet implemented */
  }

  return value;
}

} // namespace cortex
