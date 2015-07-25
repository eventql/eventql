// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <cortex-http/HeaderField.h>
#include <gtest/gtest.h>

using namespace cortex;
using namespace cortex::http;

TEST(http_HeaderField, appendValue) {
  HeaderField foo("foo", "bar");

  foo.appendValue("ten");
  ASSERT_EQ("barten", foo.value());

  foo.appendValue("er", "d");
  ASSERT_EQ("bartender", foo.value());
}

TEST(http_HeaderField, operator_EQU) {
  const HeaderField foo("foo", "bar");

  ASSERT_EQ(HeaderField("foo", "bar"), foo);
  ASSERT_EQ(HeaderField("foo", "BAR"), foo);
  ASSERT_EQ(HeaderField("FOO", "BAR"), foo);
}

TEST(http_HeaderField, operator_NE) {
  const HeaderField foo("foo", "bar");

  ASSERT_NE(HeaderField("foo", " bar "), foo);
  ASSERT_NE(HeaderField("foo", "tom"), foo);
  ASSERT_NE(HeaderField("tom", "tom"), foo);
}

TEST(http_HeaderField, inspect) {
  HeaderField field("foo", "bar");
  ASSERT_EQ("HeaderField(\"foo\", \"bar\")", inspect(field));
}
