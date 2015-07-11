/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord/web/SecureCookie.h>

namespace fnord {
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

//Option<SecureCookie> SecureCookieCoder::decodeAndVerifyWithoutExpiration(
//    const String& data) {
//
//}
//
//String SecureCookieCoder::encodeWithoutEncryption(const SecureCookie& cookie) {
//
//}

}
}

