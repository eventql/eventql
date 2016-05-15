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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/buffer.h>
#include <eventql/util/duration.h>
#include <eventql/util/exception.h>
#include <eventql/util/HMAC.h>
#include <eventql/util/option.h>
#include <eventql/util/wallclock.h>

namespace util {
namespace web {

/**
 * A secure cookie stores some data along with a timestamp. The timestamp
 * field usually contains the time at which the secure cookie was created.
 *
 * A Secure Cookie can be encoded to and from UTF-8 strings. When encoding a
 * secure cookie, the contents are checksummed using HMAC-SHA1 and optionally
 * also encrypted (see below).
 *
 * When decoding a Secure Cookie, the integrity of the data is validated by
 * checking the contained HMAC. If the cookie data was encrypted, it is
 * transparently decrypted.
 */
class SecureCookie {
public:

  /**
   * Create a new secure cookie from some plaintext data and additionally
   * an explicit created_at time
   */
  SecureCookie(
      Buffer data,
      UnixTime created_at = WallClock::now());

  /**
   * Create a new secure cookie from some plaintext data and additionally
   * an explicit created_at time
   */
  SecureCookie(
      String data,
      UnixTime created_at = WallClock::now());

  /**
   * Create a new secure cookie from some plaintext data and additionally
   * an explicit created_at time
   */
  SecureCookie(
      const void* data,
      size_t size,
      UnixTime created_at = WallClock::now());

  /**
   * Returns the plaintext data stored in this Secure Cookie
   */
  const Buffer& data() const;

  /**
   * Returns the time at which this secure cookie was created
   */
  const UnixTime& createdAt() const;

protected:
  Buffer data_;
  UnixTime created_at_;
};

class SecureCookieCoder {
public:

  static const uint64_t kDefaultExpirationTimeout = kMicrosPerDay * 365;

  SecureCookieCoder(
      String secret_key,
      const Duration& expire_after = kDefaultExpirationTimeout);

  Option<SecureCookie> decodeAndVerify(const String& data);

  Option<SecureCookie> decodeAndVerifyWithoutExpiration(const String& data);

  String encodeWithoutEncryption(const SecureCookie& cookie);

  String encodeWithAES128(const SecureCookie& cookie);

protected:
  String secret_key_;
  Buffer secret_key_buf_;
  Duration expire_after_;
};

}
}

