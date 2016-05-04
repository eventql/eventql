/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/web/SecureCookie.h"
#include "stx/test/unittest.h"

using namespace stx;

UNIT_TEST(SecureCookieTest);

TEST_CASE(SecureCookieTest, TestEncodeDecodeWithoutDecryption, [] () {
  String msg = "strenggeheim";
  String key = "qlah29uTqT2Y87411FsS5Bc45sO5jf9s";

  web::SecureCookieCoder coder(key);
  auto encoded = coder.encodeWithoutEncryption(web::SecureCookie(msg));
  auto decoded = coder.decodeAndVerify(encoded);

  EXPECT_FALSE(decoded.isEmpty());
  EXPECT_EQ(decoded.get().data().toString(), msg);
});
