// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <gtest/gtest.h>
#include <cortex-base/Buffer.h>
#include <cortex-base/Base64.h>
#include <cstdio>

using namespace cortex;

// {{{ debug helper
void print(const Buffer& b, const char* msg = 0) {
  if (msg && *msg)
    printf("\nbuffer(%s): '%s'\n", msg, b.str().c_str());
  else
    printf("\nbuffer: '%s'\n", b.str().c_str());
}

void print(const BufferRef& v, const char* msg = 0) {
  char prefix[64];
  if (msg && *msg)
    snprintf(prefix, sizeof(prefix), "\nbuffer.view(%s)", msg);
  else
    snprintf(prefix, sizeof(prefix), "\nbuffer.view");

  if (v)
    printf("\n%s: '%s' (size=%zu)\n", prefix, v.str().c_str(), v.size());
  else
    printf("\n%s: NULL\n", prefix);
}

bool testDecode(const BufferRef& decoded, const BufferRef& encoded) {
  Buffer result = Base64::decode(encoded);
  return equals(result, decoded);
}
//}}}

TEST(Base64, encode) {
  ASSERT_EQ("YQ==", Base64::encode(std::string("a")));
  ASSERT_EQ("YWI=", Base64::encode(std::string("ab")));
  ASSERT_EQ("YWJj", Base64::encode(std::string("abc")));
  ASSERT_EQ("YWJjZA==", Base64::encode(std::string("abcd")));
  ASSERT_EQ("Zm9vOmJhcg==", Base64::encode(std::string("foo:bar")));
}

TEST(Base64, decode) {
  ASSERT_EQ(true, testDecode("a", "YQ=="));
  ASSERT_EQ(true, testDecode("ab", "YWI="));
  ASSERT_EQ(true, testDecode("abc", "YWJj"));
  ASSERT_EQ(true, testDecode("abcd", "YWJjZA=="));
  ASSERT_EQ(true, testDecode("foo:bar", "Zm9vOmJhcg=="));
}
