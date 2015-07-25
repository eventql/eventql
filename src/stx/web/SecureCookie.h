/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/buffer.h>
#include <stx/duration.h>
#include <stx/exception.h>
#include <stx/HMAC.h>
#include <stx/option.h>
#include <stx/wallclock.h>

namespace stx {
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

