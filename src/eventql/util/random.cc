/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <assert.h>
#include <eventql/util/inspect.h>
#include <eventql/util/random.h>
#include <eventql/util/stringutil.h>

namespace stx {

Random::Random() {
  std::random_device r;
  prng_.seed(r() ^ time(NULL));
}

uint64_t Random::random64() {
  uint64_t rval = prng_();
  assert(rval > 0);
  return rval;
}

SHA1Hash Random::sha1() {
  auto rval = Random::hex256();
  return SHA1::compute(rval.data(), rval.size());
}

std::string Random::hex64() {
  uint64_t val = random64();
  return StringUtil::hexPrint(&val, sizeof(val), false);
}

std::string Random::hex128() {
  uint64_t val[2];
  val[0] = random64();
  val[1] = random64();

  return StringUtil::hexPrint(&val, sizeof(val), false);
}

std::string Random::hex256() {
  uint64_t val[4];
  val[0] = random64();
  val[1] = random64();
  val[2] = random64();
  val[3] = random64();

  return StringUtil::hexPrint(&val, sizeof(val), false);
}

std::string Random::hex512() {
  uint64_t val[8];
  val[0] = random64();
  val[1] = random64();
  val[2] = random64();
  val[3] = random64();
  val[4] = random64();
  val[5] = random64();
  val[6] = random64();
  val[7] = random64();

  return StringUtil::hexPrint(&val, sizeof(val), false);
}

std::string Random::alphanumericString(int nchars) {
  static const char kAlphanumericChars[] =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  std::string str;
  // FIXPAUL too many rand() calls!
  for (int i = 0; i < nchars; ++i) {
    str += kAlphanumericChars[prng_() % (sizeof(kAlphanumericChars) - 1)];
  }

  return str;
}

Random* Random::singleton() {
  static Random rnd;
  return &rnd;
}

}
