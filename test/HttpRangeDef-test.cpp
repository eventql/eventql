// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <gtest/gtest.h>
// FIXME #include <xzero/http/HttpRangeDef.h>
#include <xzero/Buffer.h>
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <cassert>
#include <strings.h>

// TODO: Property API missing (redesign HttpRangeDef or just import?)
#if 0
using xzero::HttpRangeDef;
using xzero::BufferRef;

TEST(HttpRangeDef, range1) {
  HttpRangeDef r;
  BufferRef spec("bytes=0-499");

  ASSERT_TRUE(r.parse(spec.ref()));
  ASSERT_EQ("bytes", r.unitName());
  ASSERT_EQ(1, r.size());
  ASSERT_EQ(0, r[0].first);
  ASSERT_EQ(499, r[0].second);
}

TEST(HttpRangeDef, range2) {
  HttpRangeDef r;
  BufferRef spec("bytes=500-999");

  ASSERT_TRUE(r.parse(spec.ref()));
  ASSERT_EQ("bytes", r.unitName());
  ASSERT_EQ(1, r.size());
  ASSERT_EQ(500, r[0].first);
  ASSERT_EQ(999, r[0].second);
}

TEST(HttpRangeDef, range3) {
  HttpRangeDef r;

  BufferRef spec("bytes=-500");
  ASSERT_TRUE(r.parse(spec.ref()));
  ASSERT_EQ("bytes", r.unitName());
  ASSERT_EQ(1, r.size());
  ASSERT_EQ(HttpRangeDef::npos, r[0].first);
  ASSERT_EQ(500, r[0].second);
}

TEST(HttpRangeDef, range4) {
  HttpRangeDef r;

  BufferRef spec("bytes=9500-");
  ASSERT_TRUE(r.parse(spec.ref()));
  ASSERT_EQ("bytes", r.unitName());
  ASSERT_EQ(1, r.size());
  ASSERT_EQ(9500, r[0].first);
  ASSERT_EQ(HttpRangeDef::npos, r[0].second);
}

TEST(HttpRangeDef, range5) {
  HttpRangeDef r;

  BufferRef spec("bytes=0-0,-1");
  ASSERT_TRUE(r.parse(spec.ref()));
  ASSERT_EQ("bytes", r.unitName());
  ASSERT_EQ(2, r.size());

  ASSERT_EQ(0, r[0].first);
  ASSERT_EQ(0, r[0].second);

  ASSERT_EQ(HttpRangeDef::npos, r[1].first);
  ASSERT_EQ(1, r[1].second == 1);
}

TEST(HttpRangeDef, range6) {
  HttpRangeDef r;

  BufferRef spec("bytes=500-700,601-999");
  ASSERT_TRUE(r.parse(spec.ref()));
  ASSERT_EQ("bytes", r.unitName());
  ASSERT_EQ(2, r.size());

  ASSERT_EQ(500, r[0].first);
  ASSERT_EQ(700, r[0].second);

  ASSERT_EQ(601, r[1].first);
  ASSERT_EQ(999, r[1].second);
}
#endif
