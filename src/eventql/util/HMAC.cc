/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/util/HMAC.h>

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
