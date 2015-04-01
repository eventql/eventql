/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <3rdparty/simdcomp/simdcomp.h>
#include <fnord-base/inspect.h>
#include <fnord-base/util/PFOREncoder.h>

namespace fnord {
namespace util {

PFOREncoder::PFOREncoder() : offset_(0) {
  stage_.reserve(128);
}

void PFOREncoder::encode(uint32_t value) {
  stage_.emplace_back(value);

  if (stage_.size() == SIMDBlockSize) {
    flush();
  }
}

void PFOREncoder::flush() {
  __m128i tmp[255];

  /* pad with zeroes */
  //for (int i = SIMDBlockSize - stage_.size(); i > 0; --i) {
  //  stage_.emplace_back(0);
  //}

  uint8_t b = simdmaxbitsd1(offset_, stage_.data());

  fnord::iputs("pack: $0", b);
  simdpackwithoutmaskd1(
      offset_,
      stage_.data(),
      (__m128i *) tmp,
      b);

  buf_.append(&b, sizeof(b));
  buf_.append(&tmp, b * sizeof(__m128i));
  offset_ = stage_.back();
  stage_.clear();
}

void* PFOREncoder::data() const {
  fnord::iputs("pforencoder size: $0", buf_.size());
  return buf_.data();
}

size_t PFOREncoder::size() const {
  return buf_.size();
}

}
}

