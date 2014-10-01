// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <gtest/gtest.h>
#include <xzero/Buffer.h>
#include <xzero/Base64.h>
#include <cstdio>

using namespace xzero;

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
