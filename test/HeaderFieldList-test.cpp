// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/http/HeaderFieldList.h>
#include <gtest/gtest.h>

using namespace xzero;

TEST(HeaderFieldList, initiallyEmpty) {
  HeaderFieldList headers;

  ASSERT_EQ(0, headers.size());
  ASSERT_EQ(true, headers.empty());
}

TEST(HeaderFieldList, copyList) {
  HeaderFieldList headers = {
    { "foo", "the foo" },
    { "bar", "the bar" }
  };

  HeaderFieldList copy = headers;

  ASSERT_EQ(2, headers.size());
  ASSERT_EQ("the foo", headers.get("foo"));
  ASSERT_EQ("the bar", headers.get("bar"));

  ASSERT_EQ(2, copy.size());
  ASSERT_EQ("the foo", copy.get("foo"));
  ASSERT_EQ("the bar", copy.get("bar"));
}

TEST(HeaderFieldList, moveList) {
  HeaderFieldList headers = {
    { "foo", "the foo" },
    { "bar", "the bar" }
  };

  HeaderFieldList moved = std::move(headers);

  ASSERT_EQ(0, headers.size());
  ASSERT_EQ(2, moved.size());
  ASSERT_EQ("the foo", moved.get("foo"));
  ASSERT_EQ("the bar", moved.get("bar"));
}

TEST(HeaderFieldList, get) {
  HeaderFieldList headers = {
    { "foo", "the foo" },
    { "bar", "the bar" }
  };

  ASSERT_TRUE(headers.get("foo") == "the foo");
  ASSERT_TRUE(headers.get("FOO") == "the foo");
  ASSERT_TRUE(headers.get("notfound") == "");
}

TEST(HeaderFieldList, append) {
  HeaderFieldList headers = {
    { "foo", "the foo" },
    { "bar", "the bar" }
  };

  headers.append("foo", "!");
  ASSERT_EQ("the foo!", headers.get("foo"));
}

TEST(HeaderFieldList, overwrite) {
  HeaderFieldList headers = {
    { "foo", "the foo" },
    { "bar", "the bar" }
  };

  headers.overwrite("foo", "no-foo");

  ASSERT_EQ("no-foo", headers.get("foo"));
}

TEST(HeaderFieldList, removeAll) {
  HeaderFieldList headers = {
    { "foo", "the foo" },
    { "bar", "the bar" },
    { "foo", "the foo^2" },
    { "bar", "the bar^2" }
  };

  headers.remove("foo");
  ASSERT_EQ(2, headers.size());

  headers.remove("bar");
  ASSERT_EQ(0, headers.size());
}

TEST(HeaderFieldList, containsName) {
  HeaderFieldList headers = {
    { "foo", "the foo" },
    { "bar", "the bar" },
    { "foo", "the foo^2" },
    { "bar", "the bar^2" }
  };

  ASSERT_EQ(true, headers.contains("foo"));
  ASSERT_EQ(true, headers.contains("FOO"));
  ASSERT_EQ(false, headers.contains("foos"));
}

TEST(HeaderFieldList, containsField) {
  HeaderFieldList headers = {
    { "foo", "the foo" },
    { "bar", "the bar" },
    { "foo", "the foo^2" },
    { "bar", "the bar^2" }
  };

  ASSERT_EQ(true, headers.contains("foo", "the foo^2"));
  ASSERT_EQ(true, headers.contains("FOO", "THE FOO^2"));
  ASSERT_EQ(false, headers.contains("foo", "foes"));
  ASSERT_EQ(false, headers.contains("foos"));
}
