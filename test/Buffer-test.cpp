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
#include <xzero/Buffer.h>

#include <iostream>
#include <cstdlib>
#include <cctype>
#include <cassert>
#include <strings.h>

using namespace xzero;

// {{{ BufferBase<>
TEST(BufferBase, ctor0) {
  Buffer a;

  ASSERT_EQ(0, a.size());
  ASSERT_TRUE(a.empty());
  ASSERT_TRUE(!a);
  ASSERT_TRUE(!static_cast<bool>(a));
}

TEST(BufferBase, begins) {
  BufferRef b("hello");
  BufferRef v(b.ref());

  ASSERT_TRUE(v.begins(nullptr));
  ASSERT_TRUE(v.begins(""));
  ASSERT_TRUE(v.begins("hello"));
}

TEST(BufferBase, find_cstr) {
  BufferRef buf("012345");
  BufferRef ref = buf.ref(1);

  int i = ref.find("34");
  ASSERT_EQ(2, i);

  ASSERT_EQ(0, ref.find("1"));
  ASSERT_EQ(0, ref.find("12"));
  ASSERT_EQ(0, ref.find("12345"));
  ASSERT_EQ(BufferRef::npos, ref.find("11"));
}

TEST(BufferBase, replaceAll1) {
  Buffer source("foo|bar|com");
  Buffer escaped = source.replaceAll('|', ':');

  ASSERT_EQ("foo:bar:com", escaped);
}

TEST(BufferBase, replaceAll2) {
  Buffer source("hello\nworld\n");
  Buffer replaced = source.replaceAll("\n", "<br/>");

  ASSERT_EQ("hello<br/>world<br/>", replaced.str());
}

TEST(BufferBase, toBool) {
  // true
  ASSERT_TRUE(BufferRef("true").toBool());
  ASSERT_TRUE(BufferRef("TRUE").toBool());
  ASSERT_TRUE(BufferRef("True").toBool());
  ASSERT_TRUE(BufferRef("1").toBool());

  // false
  ASSERT_TRUE(!BufferRef("false").toBool());
  ASSERT_TRUE(!BufferRef("FALSE").toBool());
  ASSERT_TRUE(!BufferRef("False").toBool());
  ASSERT_TRUE(!BufferRef("0").toBool());

  // invalid cast results into false
  ASSERT_TRUE(!BufferRef("BLAH").toBool());
}

TEST(BufferBase, iterator) {
  BufferRef b("Hello");
  BufferRef::iterator i = b.begin();
  BufferRef::iterator e = b.end();

  ASSERT_EQ('H', *i);

  ++i;
  ASSERT_TRUE(i != e);
  ASSERT_EQ('e', *i);

  ++i;
  ASSERT_TRUE(i != e);
  ASSERT_EQ('l', *i);

  ++i;
  ASSERT_TRUE(i != e);
  ASSERT_EQ('l', *i);

  ++i;
  ASSERT_TRUE(i != e);
  ASSERT_EQ('o', *i);

  ++i;
  ASSERT_TRUE(i == e);
}

// }}}
// {{{ BufferRef
TEST(BufferRef, ctor_empty) {
  BufferRef b;
  ASSERT_EQ(0, b.size());
  ASSERT_TRUE(b.empty());
  ASSERT_TRUE(b.begin() == b.end());
}

TEST(BufferRef, ctor_str_len) {
  BufferRef b("Hello", 5);

  ASSERT_EQ(5, b.size());
  ASSERT_EQ("Hello", b);
}

TEST(BufferRef, ctor_stdstring) {
  std::string s = "Hello";
  BufferRef b(s);

  ASSERT_TRUE(b.data() == s.data());
  ASSERT_TRUE(b.size() == s.size());
}

TEST(BufferRef, ctor_BufferRef) {
  BufferRef a("Hello", 5);
  BufferRef b(a);

  ASSERT_TRUE(a.data() == b.data());
  ASSERT_TRUE(a.size() == b.size());
}

TEST(BufferRef, ctor_POD) {
  BufferRef b("Hello");

  ASSERT_EQ(5, b.size());
  ASSERT_EQ("Hello", b);
}

TEST(BufferRef, operator_assign) {
  BufferRef a("Hello");
  BufferRef b;

  b = a;

  ASSERT_TRUE(a.data() == b.data());
  ASSERT_TRUE(a.size() == b.size());
}

TEST(BufferRef, operator_rndaccess) {
  BufferRef b("Hello");

  ASSERT_EQ('H', b[0]);
  ASSERT_EQ('e', b[1]);
  ASSERT_EQ('l', b[2]);
  ASSERT_EQ('l', b[3]);
  ASSERT_EQ('o', b[4]);
}

TEST(BufferRef, shl) {
  BufferRef b("Hello");

  b.shl(-1);
  ASSERT_EQ("ello", b);

  b.shl(1);
  ASSERT_EQ("Hello", b);
}

TEST(BufferRef, shr) {
  BufferRef b("Hello");

  b.shr(-1);
  ASSERT_EQ("Hell", b);

  b.shr(1);
  ASSERT_EQ("Hello", b);
}

TEST(BufferRef, reverse_iterator) {
  BufferRef b("Hello");

  BufferRef::reverse_iterator i = b.rbegin();
  BufferRef::reverse_iterator e = b.rend();

  ASSERT_TRUE(i != e);
  ASSERT_EQ('o', *i);

  ++i;
  ASSERT_TRUE(i != e);
  ASSERT_EQ('l', *i);

  ++i;
  ASSERT_TRUE(i != e);
  ASSERT_EQ('l', *i);

  ++i;
  ASSERT_TRUE(i != e);
  ASSERT_EQ('e', *i);

  ++i;
  ASSERT_TRUE(i != e);
  ASSERT_EQ('H', *i);

  ++i;
  ASSERT_TRUE(i == e);
}
// }}}
// {{{ MutableBuffer<>
TEST(MutableBuffer, resize) {
  // test modifying buffer size
  Buffer buf;
  buf.push_back("hello");
  ASSERT_EQ(5, buf.size());

  buf.resize(4);
  ASSERT_EQ(4, buf.size());
  ASSERT_EQ("hell", buf);
}

TEST(MutableBuffer, swap) {
  Buffer a("hello");
  Buffer b("world");

  a.swap(b);
  ASSERT_EQ("world", a);
  ASSERT_EQ("hello", b);

  std::swap(a, b);
  ASSERT_EQ("hello", a);
  ASSERT_EQ("world", b);
}

TEST(MutableBuffer, reserve) {
  Buffer buf;

  buf.reserve(1);
  ASSERT_EQ(0, buf.size());
  ASSERT_EQ(1, buf.capacity());

  buf.reserve(2);
  ASSERT_EQ(0, buf.size());
  ASSERT_EQ(Buffer::CHUNK_SIZE, buf.capacity());
}

TEST(MutableBuffer, clear) {
  Buffer buf("hello");

  const std::size_t capacity = buf.capacity();

  buf.clear();
  ASSERT_TRUE(buf.empty());
  ASSERT_EQ(0, buf.size());

  // shouldn't have changed internal buffer
  ASSERT_EQ(capacity, buf.capacity());
}

TEST(MutableBuffer, capacity) {
  Buffer buf;
  ASSERT_EQ(0, buf.capacity());

  buf.push_back("hello");
  ASSERT_GE(5, buf.capacity());

  buf.setCapacity(4);
  ASSERT_GE(4, buf.capacity());
  ASSERT_EQ(4, buf.size());
  ASSERT_EQ("hell", buf);
}
// }}}
// {{{ FixedBuffer
TEST(FixedBuffer, ctorVoid) {
  FixedBuffer obj;

  ASSERT_EQ(0, obj.size());
  ASSERT_EQ(0, obj.capacity());
}

TEST(FixedBuffer, ctorCopy) {
  char buf[8] = {"Hello"};
  FixedBuffer source(buf, sizeof(buf), 5);
  FixedBuffer target(source);

  // source should be empty
  ASSERT_EQ(5, source.size());
  ASSERT_EQ(sizeof(buf), source.capacity());
  ASSERT_EQ("Hello", source);
  ASSERT_EQ(buf, source.data());

  // target should contain the data
  ASSERT_EQ(5, target.size());
  ASSERT_EQ(sizeof(buf), target.capacity());
  ASSERT_EQ("Hello", target);
  ASSERT_EQ(buf, target.data());
}

TEST(FixedBuffer, ctorMove) {
  char buf[8] = {"Hello"};
  FixedBuffer source(buf, sizeof(buf), 5);
  FixedBuffer target(std::move(source));

  // source should be empty
  ASSERT_EQ(0, source.size());
  ASSERT_EQ(0, source.capacity());
  ASSERT_EQ("", source);
  ASSERT_EQ(nullptr, source.data());

  // target should contain the data
  ASSERT_EQ(5, target.size());
  ASSERT_EQ(sizeof(buf), target.capacity());
  ASSERT_EQ("Hello", target);
  ASSERT_EQ(buf, target.data());
}

TEST(FixedBuffer, ctorBuf) {
  char buf[8];
  FixedBuffer obj(buf, sizeof(buf), 0);

  ASSERT_EQ(0, obj.size());
  ASSERT_EQ(sizeof(buf), obj.capacity());
}

TEST(FixedBuffer, mutateInbound) {
  char buf[8];
  FixedBuffer obj(buf, sizeof(buf), 0);

  obj.push_back("012");

  ASSERT_EQ(3, obj.size());
  ASSERT_EQ("012", obj);
  ASSERT_EQ(0, strcmp("012", obj.c_str()));
}

TEST(FixedBuffer, mutateOverflow) {
  char buf[8];
  FixedBuffer obj(buf, sizeof(buf), 0);

  obj.push_back("0123456789");

  ASSERT_EQ(8, obj.size());
  ASSERT_EQ("01234567", obj);
  ASSERT_EQ(nullptr, obj.c_str());
}
// }}}
#if 0   // {{{ legacy tests not yet ported to gtest
// {{{ debug helper
void print(const Buffer& b, const char *msg = 0)
{
    if (msg && *msg)
        printf("buffer(%s): '%s'\n", msg, b.str().c_str());
    else
        printf("buffer: '%s'\n", b.str().c_str());
}

void print(const BufferRef& v, const char *msg = 0)
{
    if (msg && *msg)
        printf("buffer.view(%s): '%s'\n", msg, v.str().c_str());
    else
        printf("buffer.view: '%s'\n", v.str().c_str());

    printf("  size=%ld\n", v.size());
}
//}}}
// {{{ buffer tests
Buffer getbuf()
{
    //Buffer buf;
    //buf.push_back("12345");
    //return buf;
    return Buffer("12345");
}

void const_buffer1()
{
    BufferRef empty("");
    CPPUNIT_ASSERT(empty.empty());
    CPPUNIT_ASSERT(empty.size() == 0);
    CPPUNIT_ASSERT(empty == "");

    BufferRef hello("hello");
    CPPUNIT_ASSERT(!hello.empty());
    CPPUNIT_ASSERT(hello.size() == 5);
    CPPUNIT_ASSERT(hello == "hello");

    //! \todo ensure const buffers are unmutable
}

void operator_bool()
{
    Buffer buf;
    CPPUNIT_ASSERT(bool(buf) == false);

    buf.push_back("hello");
    CPPUNIT_ASSERT(bool(buf) == true);
}

void operator_not()
{
    Buffer buf;
    CPPUNIT_ASSERT(buf.operator!() == true);

    buf.push_back("hello");
    CPPUNIT_ASSERT(buf.operator!() == false);
}

void iterators()
{
    {
        Buffer buf;
        buf.push_back("hello");

        Buffer::iterator i = buf.begin();
        CPPUNIT_ASSERT(i != buf.end());
        CPPUNIT_ASSERT(*i == 'h');
        CPPUNIT_ASSERT(*++i == 'e');
        CPPUNIT_ASSERT(i != buf.end());
        CPPUNIT_ASSERT(*i++ == 'e');
        CPPUNIT_ASSERT(*i == 'l');
        CPPUNIT_ASSERT(i != buf.end());
        CPPUNIT_ASSERT(*++i == 'l');
        CPPUNIT_ASSERT(i != buf.end());
        CPPUNIT_ASSERT(*++i == 'o');
        CPPUNIT_ASSERT(i != buf.end());
        CPPUNIT_ASSERT(++i == buf.end());
    }

    {
        Buffer buf;
        buf.push_back("hello");

        for (Buffer::iterator i = buf.begin(); i != buf.end(); ++i)
            *i = std::toupper(*i);

        CPPUNIT_ASSERT(buf == "HELLO");
    }
}

void const_iterators()
{
    Buffer buf;
    buf.push_back("hello");

    BufferRef::const_iterator i = buf.cbegin();
    CPPUNIT_ASSERT(i != buf.cend());
    CPPUNIT_ASSERT(*i == 'h');
    CPPUNIT_ASSERT(*++i == 'e');
    CPPUNIT_ASSERT(i != buf.cend());
    CPPUNIT_ASSERT(*i++ == 'e');
    CPPUNIT_ASSERT(*i == 'l');
    CPPUNIT_ASSERT(i != buf.cend());
    CPPUNIT_ASSERT(*++i == 'l');
    CPPUNIT_ASSERT(i != buf.cend());
    CPPUNIT_ASSERT(*++i == 'o');
    CPPUNIT_ASSERT(i != buf.cend());
    CPPUNIT_ASSERT(++i == buf.cend());
}

void push_back()
{
    Buffer buf;

    buf.push_back('h');
    CPPUNIT_ASSERT(buf == "h");


    buf.push_back("");
    CPPUNIT_ASSERT(buf == "h");
    buf.push_back("e");
    CPPUNIT_ASSERT(buf == "he");

    buf.push_back("llo");
    CPPUNIT_ASSERT(buf == "hello");

    std::string s(" world");
    buf.push_back(s);
    CPPUNIT_ASSERT(buf == "hello world");

    buf.clear();
    buf.push_back(s.data(), s.size());
    CPPUNIT_ASSERT(buf == " world");
}

void random_access()
{
    // test operator[](...)
}

void ref()
{
    BufferRef a("hello");

    CPPUNIT_ASSERT(a == "hello");
    CPPUNIT_ASSERT(a.ref(0) == "hello");
    CPPUNIT_ASSERT(a.ref(1) == "ello");
    CPPUNIT_ASSERT(a.ref(2) == "llo");
    CPPUNIT_ASSERT(a.ref(5) == "");
}

void std_string()
{
    // test std::string() utility functions

    BufferRef a("hello");
    std::string s(a.str());

    CPPUNIT_ASSERT(a.data() != s.data());
    CPPUNIT_ASSERT(a.size() == s.size());
    CPPUNIT_ASSERT(s == "hello");
}
// }}}
// {{{ BufferRef tests
void ref_as_int()
{
    CPPUNIT_ASSERT(BufferRef("1234").toInt() == 1234);

    CPPUNIT_ASSERT(BufferRef("-1234").toInt() == -1234);
    CPPUNIT_ASSERT(BufferRef("+1234").toInt() == +1234);

    CPPUNIT_ASSERT(BufferRef("12.34").toInt() == 12);
}

void ref_hex()
{
    CPPUNIT_ASSERT(BufferRef("1234").hex<int>() == 0x1234);
    CPPUNIT_ASSERT(BufferRef("5678").hex<int>() == 0x5678);

    CPPUNIT_ASSERT(BufferRef("abcdef").hex<int>() == 0xabcdef);
    CPPUNIT_ASSERT(BufferRef("ABCDEF").hex<int>() == 0xABCDEF);

    CPPUNIT_ASSERT(BufferRef("ABCDEFG").hex<int>() == 0xABCDEF);
    CPPUNIT_ASSERT(BufferRef("G").hex<int>() == 0);
}
// }}}
#endif  // }}}
