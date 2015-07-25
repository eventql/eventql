// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <gtest/gtest.h>
#include <cortex-base/Uri.h>
#include <stdlib.h>
#include <stdio.h>

using namespace cortex;

TEST(Uri, SchemeAndAuthority) {
  Uri uri("fnord://myhost");

  EXPECT_EQ(uri.scheme(), "fnord");
  EXPECT_EQ(uri.host(), "myhost");
  EXPECT_EQ(uri.port(), 0);

  EXPECT_EQ(uri.toString(), "fnord://myhost");
}

TEST(Uri, SchemeAndAuthorityWithPort) {
  Uri uri("fnord://myhost:2345");

  EXPECT_EQ(uri.scheme(), "fnord");
  EXPECT_EQ(uri.host(), "myhost");
  EXPECT_EQ(uri.port(), 2345);

  EXPECT_EQ(uri.toString(), "fnord://myhost:2345");
}

TEST(Uri, SchemeAndAuthorityWithUserInfo) {
  Uri uri("fnord://blah@myhost");

  EXPECT_EQ(uri.scheme(), "fnord");
  EXPECT_EQ(uri.userinfo(), "blah");
  EXPECT_EQ(uri.host(), "myhost");
  EXPECT_EQ(uri.port(), 0);

  EXPECT_EQ(uri.toString(), "fnord://blah@myhost");
}

TEST(Uri, SchemeAndAuthorityWithUserInfoWithPort) {
  Uri uri("fnord://blah@myhost:2345");

  EXPECT_EQ(uri.scheme(), "fnord");
  EXPECT_EQ(uri.userinfo(), "blah");
  EXPECT_EQ(uri.host(), "myhost");
  EXPECT_EQ(uri.port(), 2345);

  EXPECT_EQ(uri.toString(), "fnord://blah@myhost:2345");
}

TEST(Uri, SchemeAndAuthorityWithUserInfoSub) {
  Uri uri("fnord://blah:asd@myhost");

  EXPECT_EQ(uri.scheme(), "fnord");
  EXPECT_EQ(uri.userinfo(), "blah:asd");
  EXPECT_EQ(uri.host(), "myhost");
  EXPECT_EQ(uri.port(), 0);

  EXPECT_EQ(uri.toString(), "fnord://blah:asd@myhost");
}

TEST(Uri, SchemeAndAuthorityWithUserInfoSubWithPort) {
  Uri uri("fnord://blah:asd@myhost:2345");

  EXPECT_EQ(uri.scheme(), "fnord");
  EXPECT_EQ(uri.userinfo(), "blah:asd");
  EXPECT_EQ(uri.host(), "myhost");
  EXPECT_EQ(uri.port(), 2345);

  EXPECT_EQ(uri.toString(), "fnord://blah:asd@myhost:2345");
}

TEST(Uri, SchemeAndPath) {
  Uri uri("fnord:my/path");

  EXPECT_EQ(uri.scheme(), "fnord");
  EXPECT_EQ(uri.path(), "my/path");

  EXPECT_EQ(uri.toString(), "fnord:my/path");
}

TEST(Uri, SchemeAndPathAndQuery) {
  Uri uri("fnord:my/path?asdasd");

  EXPECT_EQ(uri.scheme(), "fnord");
  EXPECT_EQ(uri.path(), "my/path");
  EXPECT_EQ(uri.query(), "asdasd");

  EXPECT_EQ(uri.toString(), "fnord:my/path?asdasd");
}

TEST(Uri, SchemeAndPathWithLeadingSlash) {
  Uri uri("fnord:/my/path");

  EXPECT_EQ(uri.scheme(), "fnord");
  EXPECT_EQ(uri.path(), "/my/path");

  EXPECT_EQ(uri.toString(), "fnord:/my/path");
}

TEST(Uri, SchemeAndPathWithQueryString) {
  Uri uri("fnord:/my/path?myquerystring");

  EXPECT_EQ(uri.scheme(), "fnord");
  EXPECT_EQ(uri.path(), "/my/path");
  EXPECT_EQ(uri.query(), "myquerystring");

  EXPECT_EQ(uri.toString(), "fnord:/my/path?myquerystring");
}

TEST(Uri, SchemeAndPathWithQueryStringAndFragment) {
  Uri uri("fnord:/my/path?myquerystring#myfragment");

  EXPECT_EQ(uri.scheme(), "fnord");
  EXPECT_EQ(uri.path(), "/my/path");
  EXPECT_EQ(uri.query(), "myquerystring");
  EXPECT_EQ(uri.fragment(), "myfragment");

  EXPECT_EQ(uri.toString(), "fnord:/my/path?myquerystring#myfragment");
}

TEST(Uri, SchemeAndPathWithFragment) {
  Uri uri("fnord:/my/path#myfragment");

  EXPECT_EQ(uri.scheme(), "fnord");
  EXPECT_EQ(uri.path(), "/my/path");
  EXPECT_EQ(uri.fragment(), "myfragment");

  EXPECT_EQ(uri.toString(), "fnord:/my/path#myfragment");
}

TEST(Uri, ParseQueryParamsSingle) {
  Uri uri("fnord:path?fuu=bar");

  auto params = uri.queryParams();
  EXPECT_EQ(params.size(), 1);
  EXPECT_EQ(params[0].first, "fuu");
  EXPECT_EQ(params[0].second, "bar");

  EXPECT_EQ(uri.toString(), "fnord:path?fuu=bar");
}

TEST(Uri, ParseQueryParams) {
  Uri uri("fnord:path?fuu=bar&blah=123123");

  auto params = uri.queryParams();
  EXPECT_EQ(params.size(), 2);
  EXPECT_EQ(params[0].first, "fuu");
  EXPECT_EQ(params[0].second, "bar");
  EXPECT_EQ(params[1].first, "blah");
  EXPECT_EQ(params[1].second, "123123");

  EXPECT_EQ(uri.toString(), "fnord:path?fuu=bar&blah=123123");
}

TEST(Uri, WeirdUrls) {
  Uri uri1(
      "/t.gif?c=f9765c4564e077c0cb~4ae4a27f81fa&e=q&qstr:de=x" \
          "xx&is=p~40938238~1,p~70579299~2");

  EXPECT_EQ(uri1.path(), "/t.gif");
  EXPECT_EQ(
      uri1.query(),
      "c=f9765c4564e077c0cb~4ae4a27f81fa&e=q&qstr:de=x" \
          "xx&is=p~40938238~1,p~70579299~2");
}
