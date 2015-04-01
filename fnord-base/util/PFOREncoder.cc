/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/util/PFOREncoder.h>

namespace fnord {
namespace util {

void PFOREncoder::encode(uint32_t value) {
  buf_.append(&value, sizeof(value));
}

void PFOREncoder::flush() {

}

void* PFOREncoder::data() const {
  return buf_.data();
}

size_t PFOREncoder::size() const {
  return buf_.size();
}

}
}

