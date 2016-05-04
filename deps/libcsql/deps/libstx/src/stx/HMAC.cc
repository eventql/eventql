/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/HMAC.h>

namespace stx {

const size_t HMAC::kBlockSize = 64;
const char HMAC::kOPad = 0x5c;
const char HMAC::kIPad = 0x36;

SHA1Hash HMAC::hmac_sha1(const Buffer& key, const Buffer& message) {
  Buffer key_pad;

  if (key.size() > kBlockSize) {
    auto sha1 = SHA1::compute(key);
    key_pad.append(sha1.data(), sha1.size());
  } else {
    key_pad = key;
  }

  while (key_pad.size() < kBlockSize) {
    key_pad.append('\0');
  }

  Buffer opad(kBlockSize);
  Buffer ipad(kBlockSize);

  for (size_t i = 0; i < kBlockSize; ++i) {
    *opad.structAt<uint8_t>(i) = 0x5c ^ *key_pad.structAt<uint8_t>(i);
    *ipad.structAt<uint8_t>(i) = 0x36 ^ *key_pad.structAt<uint8_t>(i);
  }

  Buffer h1;
  h1.append(ipad);
  h1.append(message);

  Buffer h2;
  h2.append(opad);
  h2.append(SHA1::compute(h1).data(), SHA1Hash::kSize);

  return SHA1::compute(h2);
}

}

