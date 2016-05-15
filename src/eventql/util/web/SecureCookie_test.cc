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
#include "eventql/util/web/SecureCookie.h"
#include "eventql/util/test/unittest.h"

using namespace util;

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
