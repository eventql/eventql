// This file is part of the "x0" project
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <gtest/gtest.h>
#include <xzero/hpack.h>

using namespace base;
using namespace xzero;
using namespace xzero::hpack;

// {{{ HeaderField
TEST(hpack_HeaderField, ctor_v) {
  HeaderField field;
  ASSERT_EQ("", field.name);
  ASSERT_EQ("", field.value);
}

TEST(hpack_HeaderField, ctor_initializer_list) {
  HeaderField field = { "foo", "bar" };
  ASSERT_EQ("foo", field.name);
  ASSERT_EQ("bar", field.value);
}

TEST(hpack_HeaderField, ctor_explicit) {
  HeaderField field("foo", "bar");
  ASSERT_EQ("foo", field.name);
  ASSERT_EQ("bar", field.value);
}

TEST(hpack_HeaderField, copy) {
  HeaderField field("foo", "bar");
  HeaderField other = field;
  ASSERT_EQ("foo", other.name);
  ASSERT_EQ("bar", other.value);
}

TEST(hpack_HeaderField, move) {
  HeaderField field("foo", "bar");
  HeaderField other = std::move(field);

  ASSERT_EQ("", field.name);
  ASSERT_EQ("", field.value);

  ASSERT_EQ("foo", other.name);
  ASSERT_EQ("bar", other.value);
}

TEST(hpack_HeaderField, eq) {
  HeaderField field("foo", "bar");
  HeaderField other = field;

  ASSERT_EQ(field, other);
}

TEST(hpack_HeaderField, ne) {
  HeaderField field("foo", "bar");
  
  // value differs
  HeaderField other(field.name, "nar");
  ASSERT_NE(field, other);

  // name differs
  other = {"boo", field.value};
  ASSERT_NE(field, other);
}
// }}}
// {{{ HeaderTable
TEST(hpack_HeaderTable, initiallyEmpty) {
  HeaderTable table(10);
  ASSERT_EQ(true, table.empty());
}

TEST(hpack_HeaderTable, add) {
  HeaderTable table(10);
  ASSERT_EQ(true, table.empty());
  ASSERT_EQ(0, table.size());

  table.add({":host", "blah"});
  ASSERT_EQ(1, table.size());
  ASSERT_EQ(":host", table.entry(1)->name);

  table.add({"user-agent", "blubb"});
  ASSERT_EQ(2, table.size());
  ASSERT_EQ("user-agent", table.entry(1)->name);
  ASSERT_EQ(":host", table.entry(2)->name);
}

TEST(hpack_HeaderTable, first) {
  HeaderTable table(10);
  ASSERT_EQ(true, table.empty());
  ASSERT_EQ(0, table.size());

  table.add({":host", "blah"});
  ASSERT_EQ(1, table.size());
  ASSERT_EQ(":host", table.first().name);

  table.add({"user-agent", "blubb"});
  ASSERT_EQ(2, table.size());
  ASSERT_EQ("user-agent", table.first().name);
}

TEST(hpack_HeaderTable, last) {
  HeaderTable table(10);
  ASSERT_EQ(true, table.empty());
  ASSERT_EQ(0, table.size());

  table.add({":host", "blah"});
  ASSERT_EQ(1, table.size());
  ASSERT_EQ(":host", table.last().name);

  table.add({"user-agent", "blubb"});
  ASSERT_EQ(2, table.size());
  ASSERT_EQ(":host", table.last().name);
}

TEST(hpack_HeaderTable, setMaxEntries) {
  HeaderTable table(2);
  ASSERT_EQ(2, table.maxEntries());

  table.add({"foo", "the foo"});
  table.add({"bar", "the bar"});
  ASSERT_EQ(2, table.size());

  // shrink
  table.setMaxEntries(1);
  ASSERT_EQ(1, table.maxEntries());
  ASSERT_EQ(1, table.size());
  ASSERT_EQ("bar", table.first().name);

  // advance
  table.setMaxEntries(3);
  ASSERT_EQ(1, table.size());
  ASSERT_EQ(3, table.maxEntries());
}

TEST(hpack_HeaderTable, overcommit) {
  HeaderTable table(1);

  table.add({"foo", "the foo"});
  table.add({"bar", "the bar"});
  ASSERT_EQ(1, table.size());
  ASSERT_EQ("bar", table.first().name);

  table.add({"fnord", "the fnord"});
  ASSERT_EQ(1, table.size());
  ASSERT_EQ("fnord", table.first().name);
}

TEST(hpack_HeaderTable, clear) {
  HeaderTable table(42);
  table.add({"foo", "the foo"});
  table.add({"bar", "the bar"});

  table.clear();

  ASSERT_EQ(0, table.size());
  ASSERT_EQ(42, table.maxEntries());
}

// }}}
// {{{ ReferenceSet

// The reference set is initially empty.
TEST(hpack_ReferenceSet, isInitiallyEmpty) {
  HeaderTable table(10);
  table.add({"foo", "the foo"});

  ReferenceSet rs(&table);
  ASSERT_TRUE(rs.empty());
  ASSERT_EQ(0, rs.size());
}

TEST(hpack_ReferenceSet, itNeverContainsDuplicateEntries) {
  HeaderTable table(10);
  table.add({"foo", "the foo"});

  ReferenceSet rs(&table);
  rs.add(table.entry(1));
  rs.add(table.entry(1));
  ASSERT_EQ(1, rs.size());
}

// Adding an entry to the reference set that is not available
// in its header table is invalid.
TEST(hpack_ReferenceSet, addNotFound) {
  HeaderTable table(10);
  table.add({"foo", "the foo"});

  HeaderField fnord = {"fnord", "car"};
  ReferenceSet rs(&table);
  rs.add(&fnord);

  ASSERT_EQ(0, rs.size());
  ASSERT_FALSE(rs.contains(&fnord));
}

// When an entry is evicted from the header table, if it was referenced
// from the reference set, its reference is removed from the reference
// set.
TEST(hpack_ReferenceSet, eviction) {
  HeaderTable table(10);
  table.add({"foo", "the foo"});
  table.add({"bar", "the bar"});
  table.add({"boo", "the boo"});

  ReferenceSet rs(&table);
  rs.add(&*table.find("foo"));
  rs.add(&*table.find("boo"));

  ASSERT_EQ(2, rs.size());
  ASSERT_TRUE(rs.contains(table.entry(1)));

  table.remove(&*table.find("foo"));

  ASSERT_EQ(1, rs.size());
  ASSERT_FALSE(rs.contains(&*table.find("foo")));
}

// iterating over a reference set should work as expected.
TEST(hpack_ReferenceSet, iterator) {
  HeaderTable table(10);
  table.add({"foo", "the foo"}); // #3
  table.add({"bar", "the bar"}); // #2
  table.add({"boo", "the boo"}); // #1

  ReferenceSet rs(&table);
  rs.add(table.entry(3));
  rs.add(table.entry(2));

  unsigned fooFound = 0;
  unsigned barFound = 0;
  unsigned notFound = 0;

  for (const HeaderField& field: rs) {
    if (field.name == "foo")
      ++fooFound;
    else if (field.name == "bar")
      ++barFound;
    else
      ++notFound;
  }

  ASSERT_EQ(1, fooFound);
  ASSERT_EQ(1, barFound);
  ASSERT_EQ(0, notFound);
}

TEST(hpack_ReferenceSet, iteratorEmpty) {
  HeaderTable table(10);
  table.add({"foo", "the foo"});
  table.add({"bar", "the bar"});
  table.add({"boo", "the boo"});

  ReferenceSet rs(&table);

  ASSERT_TRUE(rs.begin() == rs.end());

  size_t iteratorCount = 0;
  for (const HeaderField& field: rs) {
    ++iteratorCount;
    (void) field;
  }
  ASSERT_EQ(0, iteratorCount);
}
// }}}
// {{{ Encoder
TEST(hpack_Encoder, encodeInt_0x00) {
  Buffer buf;
  EncoderHelper::encodeInt(&buf, 0x00, 8);

  ASSERT_EQ(1, buf.size());
  ASSERT_EQ('\x00', buf[0]);

  // decode
  unsigned nb = 0;
  const uint64_t decoded = DecoderHelper::decodeInt(buf, 8, &nb);

  ASSERT_EQ(1, nb);
  ASSERT_EQ(0x00, decoded);
}

TEST(hpack_Encoder, encodeInt_0xFFFFFF) {
  Buffer buf;
  EncoderHelper::encodeInt(&buf, 0xFFFFFF, 8);

  ASSERT_EQ(5, buf.size());
  ASSERT_EQ('\xFF', buf[0]);
  ASSERT_EQ('\x80', buf[1]);
  ASSERT_EQ('\xFE', buf[2]);
  ASSERT_EQ('\xFF', buf[3]);
  ASSERT_EQ('\x07', buf[4]);

  // decode
  unsigned nb = 0;
  const uint64_t decoded = DecoderHelper::decodeInt(buf, 8, &nb);

  ASSERT_EQ(5, nb);
  ASSERT_EQ(0xFFFFFF, decoded);
}

TEST(hpack_Encoder, encodeInt8Bit_fit) {
  // encode
  Buffer buf;
  EncoderHelper::encodeInt(&buf, 57, 8);
  BufferRef ref = BufferRef((char*)buf.data(), buf.size());

  // decode
  unsigned nb = 0;
  const uint64_t decoded = DecoderHelper::decodeInt(ref, 8, &nb);

  ASSERT_EQ(57, decoded);
  ASSERT_EQ(1, nb);
}

TEST(hpack_Encoder, encodeInt8Bit_nofit2) {
  // encode
  Buffer buf;
  EncoderHelper::encodeInt(&buf, 356, 8);
  BufferRef ref = BufferRef((char*)buf.data(), buf.size());

  // decode
  unsigned nb = 0;
  const uint64_t decoded = DecoderHelper::decodeInt(ref, 8, &nb);

  ASSERT_EQ(356, decoded);
  ASSERT_EQ(2, nb);
}

TEST(hpack_Encoder, encodeInt8Bit_nofit4) {
  // encode
  Buffer buf;
  EncoderHelper::encodeInt(&buf, 0x12345678llu, 8);

  ASSERT_EQ(6, buf.size());
  ASSERT_EQ('\xFF', buf[0]);
  ASSERT_EQ('\xF9', buf[1]);
  ASSERT_EQ('\xAA', buf[2]);
  ASSERT_EQ('\xD1', buf[3]);
  ASSERT_EQ('\x91', buf[4]);
  ASSERT_EQ('\x01', buf[5]);

  // decode
  BufferRef ref = BufferRef((char*)buf.data(), buf.size());
  unsigned nb = 0;
  const uint64_t decoded = DecoderHelper::decodeInt(ref, 8, &nb);

  ASSERT_EQ(6, nb);
  ASSERT_EQ(0x12345678llu, decoded);
}

// Appending D.1.2) encode 1337 with 5bit prefix
TEST(hpack_Encoder, encodeInt_1337_5bit) {
  Buffer buf;
  EncoderHelper::encodeInt(&buf, 1337, 5);

  ASSERT_EQ(3, buf.size());
  ASSERT_EQ('\x1F', buf[0]);
  ASSERT_EQ('\x9A', buf[1]);
  ASSERT_EQ('\x0A', buf[2]);

  unsigned nb = 0;
  const uint64_t decoded = DecoderHelper::decodeInt(buf, 5, &nb);
  ASSERT_EQ(1337, decoded);
  ASSERT_EQ(3, nb);
}
// }}}
// {{{ Decoder
TEST(hpack_Decoder, example_2) {}
// }}}
// {{{ misc
TEST(hpack_misc, test1) {
  HeaderSet hs;

  hs.push_back({"foo", "bar"});
  hs.push_back({"foo", "fnord"});
}
// }}}

// vim:ts=2:sw=2
