/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/util/Base64.h>
#include <stx/web/SecureCookie.h>

namespace stx {
namespace web {

SecureCookie::SecureCookie(
    Buffer data,
    UnixTime created_at /* = WallClock::now() */) :
    data_(data),
    created_at_(created_at) {}

SecureCookie::SecureCookie(
    String data,
    UnixTime created_at /* = WallClock::now() */) :
    data_(data.data(), data.size()),
    created_at_(created_at) {}

SecureCookie::SecureCookie(
    const void* data,
    size_t size,
    UnixTime created_at /* = WallClock::now() */) :
    data_(data, size),
    created_at_(created_at) {}

const Buffer& SecureCookie::data() const {
  return data_;
}

const UnixTime& SecureCookie::createdAt() const {
  return created_at_;
}

SecureCookieCoder::SecureCookieCoder(
    String secret_key,
    const Duration& expire_after) :
    secret_key_(secret_key),
    secret_key_buf_(secret_key.data(), secret_key.size()),
    expire_after_(expire_after) {}

Option<SecureCookie> SecureCookieCoder::decodeAndVerify(const String& data) {
  auto cookie = decodeAndVerifyWithoutExpiration(data);
  if (cookie.isEmpty()) {
    return cookie;
  }

  auto now = WallClock::unixMicros();
  auto created_at = cookie.get().createdAt().unixMicros();

  if (created_at < now && (now - created_at) > expire_after_.microseconds()) {
    return None<SecureCookie>();
  } else {
    return cookie;
  }
}

Option<SecureCookie> SecureCookieCoder::decodeAndVerifyWithoutExpiration(
    const String& data) {
  auto parts = StringUtil::split(data, "|");
  if (parts.size() != 5) {
    return None<SecureCookie>();
  }

  auto pad_end = StringUtil::findLast(data, '|');
  auto pad_hmac = HMAC::hmac_sha1(
      secret_key_buf_,
      Buffer(data.data(), pad_end + 1));

  const auto& ciphertext = parts[0];
  UnixTime created_at = std::stoull(parts[1]);
  const auto& algo = parts[2];
  const auto& iv = parts[3];
  const auto& hmac = SHA1Hash::fromHexString(parts[4]);

  if (!(pad_hmac == hmac)) {
    return None<SecureCookie>();
  }

  if (algo == "PLAIN") {
    String plaintext;
    util::Base64::decode(ciphertext, &plaintext);
    return Some(SecureCookie(plaintext, created_at));
  }

  return None<SecureCookie>();
}

String SecureCookieCoder::encodeWithoutEncryption(const SecureCookie& cookie) {
  Buffer out;
  out.append(util::Base64::encode(cookie.data().data(), cookie.data().size()));
  out.append('|');
  out.append(StringUtil::toString(cookie.createdAt().unixMicros()));
  out.append("|PLAIN||");

  auto hmac = HMAC::hmac_sha1(secret_key_buf_, out);
  out.append(hmac.toString());

  return out.toString();
}

}
}

