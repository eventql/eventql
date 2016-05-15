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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "eventql/util/io/inputstream.h"
#include "eventql/util/json/flatjsonreader.h"
#include "eventql/util/json/jsondocument.h"
#include "eventql/util/json/jsonutil.h"
#include "eventql/util/json/jsoninputstream.h"
#include "eventql/util/json/jsonpointer.h"
#include "eventql/util/test/unittest.h"

UNIT_TEST(JSONTest);

using json::FlatJSONReader;
using json::JSONDocument;
using json::JSONInputStream;
using json::JSONPointer;
using json::JSONUtil;

TEST_CASE(JSONTest, TestJSONInputStream, [] () {
  auto json1 = "{ 123: \"fnord\", \"blah\": [ true, false, null, 3.7e-5 ] }";
  JSONInputStream json1_stream(StringInputStream::fromString(json1));

  json::kTokenType token_type;
  std::string token_str;

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_OBJECT_BEGIN);

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_NUMBER);
  EXPECT_EQ(token_str, "123");

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_STRING);
  EXPECT_EQ(token_str, "fnord");

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_STRING);
  EXPECT_EQ(token_str, "blah");

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_ARRAY_BEGIN);

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_TRUE);

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_FALSE);

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_NULL);

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_NUMBER);
  EXPECT_EQ(token_str, "3.7e-5");

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_ARRAY_END);

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_OBJECT_END);
});

TEST_CASE(JSONTest, TestJSONInputStreamEscaping, [] () {
  auto json1 = "{ 123: \"fno\\\"rd\" }";
  JSONInputStream json1_stream(StringInputStream::fromString(json1));

  json::kTokenType token_type;
  std::string token_str;

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_OBJECT_BEGIN);

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_NUMBER);
  EXPECT_EQ(token_str, "123");

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_STRING);
  EXPECT_EQ(token_str, "fno\"rd");

  EXPECT_TRUE(json1_stream.readNextToken(&token_type, &token_str));
  EXPECT_EQ(token_type, json::JSON_OBJECT_END);
});

TEST_CASE(JSONTest, TestJSONDocumentGet, [] () {
  auto json1 = "{ 123: \"fnord\", \"blah\": [ true, false, null, 3.7e-5 ] }";
  JSONDocument json1_doc(json1);

  EXPECT_EQ(json1_doc.get("/123"), "fnord");
  EXPECT_EQ(json1_doc.get("/blah/0"), "true");
  EXPECT_EQ(json1_doc.get("/blah/1"), "false");
  EXPECT_EQ(json1_doc.get("/blah/2"), "null");
  EXPECT_EQ(json1_doc.get("/blah/3"), "3.7e-5");
});

TEST_CASE(JSONTest, TestJSONDocumentForEach, [] () {
  auto json1 = "{ 123: \"fnord\", \"blah\": [ true, false, null, 3.7e-5 ] }";
  JSONDocument json1_doc(json1);

  int n = 0;
  json1_doc.forEach(JSONPointer("/"), [&n] (const JSONPointer& path) -> bool {
    if (n == 0) { EXPECT_EQ(path.toString(), "/123"); }
    if (n == 1) { EXPECT_EQ(path.toString(), "/blah"); }
    EXPECT_TRUE(n++ < 2);
    return true;
  });

  n = 0;
  json1_doc.forEach(JSONPointer("/blah"), [&n] (
      const JSONPointer& path) -> bool {
    if (n == 0) { EXPECT_EQ(path.toString(), "/blah/0"); }
    if (n == 1) { EXPECT_EQ(path.toString(), "/blah/1"); }
    if (n == 2) { EXPECT_EQ(path.toString(), "/blah/2"); }
    if (n == 3) { EXPECT_EQ(path.toString(), "/blah/3"); }
    EXPECT_TRUE(n++ < 4);
    return true;
  });

  n = 0;
  json1_doc.forEach(JSONPointer("/blah/"), [&n] (
      const JSONPointer& path) -> bool {
    if (n == 0) { EXPECT_EQ(path.toString(), "/blah/0"); }
    if (n == 1) { EXPECT_EQ(path.toString(), "/blah/1"); }
    if (n == 2) { EXPECT_EQ(path.toString(), "/blah/2"); }
    if (n == 3) { EXPECT_EQ(path.toString(), "/blah/3"); }
    EXPECT_TRUE(n++ < 4);
    return true;
  });
});

TEST_CASE(JSONTest, TestJSONPointerEscaping, [] () {
  std::string str1 = "blah~fnord/bar/12~35";
  JSONPointer::escape(&str1);
  EXPECT_EQ(str1, "blah~0fnord~1bar~112~035");
});

typedef std::vector<std::pair<std::string, std::string>> VectorOfStringPairs;

TEST_CASE(JSONTest, TestFlatJSONReader, [] () {
  std::string str1 = "blah~fnord/bar/12~35";
  JSONPointer::escape(&str1);
  EXPECT_EQ(str1, "blah~0fnord~1bar~112~035");

  auto json1 = "{ 123: \"fnord\", \"blah\": [ true, false, null, 3.7e-5 ] }";
  FlatJSONReader json1_reader(
      JSONInputStream(StringInputStream::fromString(json1)));

  VectorOfStringPairs expected;
  expected.emplace_back(std::make_pair("/123", "fnord"));
  expected.emplace_back(std::make_pair("/blah/0", "true"));
  expected.emplace_back(std::make_pair("/blah/1", "false"));
  expected.emplace_back(std::make_pair("/blah/2", "null"));
  expected.emplace_back(std::make_pair("/blah/3", "3.7e-5"));

  int n = 0;
  json1_reader.read([&] (
      const JSONPointer& path,
      const std::string& value) -> bool {
    EXPECT_TRUE(n < expected.size());
    EXPECT_EQ(expected[n].first, path.toString());
    EXPECT_EQ(expected[n].second, value);
    n++;
    return true;
  });
});

TEST_CASE(JSONTest, TestFromJSON, [] () {
  EXPECT_EQ(json::fromJSON<std::string>("\"fnord\""), "fnord");

  auto json1 = json::parseJSON(
      "{ 123: \"fnord\", \"blubb\": \"xxx\", \"blah\": [ true, false, null, " \
      "3.7e-5 ] }");

  auto iter = JSONUtil::objectLookup(json1.begin(), json1.end(), "blubb");
  EXPECT_TRUE(iter != json1.end());
  EXPECT_EQ(json::fromJSON<std::string>(iter, json1.end()), "xxx");

  iter = JSONUtil::objectLookup(json1.begin(), json1.end(), "blah");
  EXPECT_TRUE(iter != json1.end());
  EXPECT_EQ(iter->type, json::JSON_ARRAY_BEGIN);
  EXPECT_EQ(JSONUtil::arrayLength(iter, json1.end()), 4);

  auto aiter = JSONUtil::arrayLookup(iter, json1.end(), 0);
  EXPECT_EQ(json::fromJSON<std::string>(aiter, json1.end()), "true");
  aiter = JSONUtil::arrayLookup(iter, json1.end(), 1);
  EXPECT_EQ(json::fromJSON<std::string>(aiter, json1.end()), "false");
  aiter = JSONUtil::arrayLookup(iter, json1.end(), 2);
  EXPECT_EQ(json::fromJSON<std::string>(aiter, json1.end()), "null");
  aiter = JSONUtil::arrayLookup(iter, json1.end(), 3);
  EXPECT_EQ(json::fromJSON<std::string>(aiter, json1.end()), "3.7e-5");
});

TEST_CASE(JSONTest, TestToJSON, [] () {
  json::JSONObject j1;
  json::toJSON(std::string("blah"), &j1);

  EXPECT_EQ(j1.size(), 1);
  EXPECT_EQ(j1[0].type, json::JSON_STRING);
  EXPECT_EQ(j1[0].data, "blah");
});

struct TestMessage {
  std::string a;
  int b;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&TestMessage::a, 1, "a", false);
    meta->prop(&TestMessage::b, 2, "b", false);
  }
};

TEST_CASE(JSONTest, TestToFromJSON, [] () {
  TestMessage m1;
  m1.a = "stringdata";
  m1.b = 23;

  auto j1 = json::parseJSON(json::toJSONString(m1));
  auto m2 = json::fromJSON<TestMessage>(j1);

  EXPECT_EQ(m1.a, m2.a);
  EXPECT_EQ(m1.b, m2.b);
});

struct TestStruct {
  String str;
  json::JSONObject obj;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&TestStruct::str, 1, "str", false);
    meta->prop(&TestStruct::obj, 2, "obj", false);
  };
};

TEST_CASE(JSONTest, TestJSONShouldNotSegfaultOnInvalidInput, [] () {
  EXPECT_EXCEPTION("unbalanced braces", [] {
    auto orig_str = "{ \"str\": \"fnord\", \"obj\": { \"a\": 1, \"b\": 2 ] } }";
    auto obj = json::fromJSON<TestStruct>(orig_str);
  });
});

TEST_CASE(JSONTest, TestJSONReEncodingViaReflectionWithObject, [] () {
  auto orig_str = "{\"str\":\"fnord\",\"obj\":{\"a\":\"1\",\"b\":\"2\"}}";
  auto obj = json::fromJSON<TestStruct>(orig_str);
  auto reenc_str = json::toJSONString(obj);
  EXPECT_EQ(orig_str, reenc_str);
});

struct TestJSONObject {
  std::string data;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&TestJSONObject::data, 1, "data", false);
  }
};


TEST_CASE(JSONTest, ParseMultiLevelEscaping, [] () {
  auto orig_str = R"({"str":"fub \\\"blah\\\" bar"})";
  auto doc = json::parseJSON(orig_str);
  EXPECT_EQ(doc.size(), 4);
  EXPECT_EQ(doc[2].type, json::JSON_STRING);
  EXPECT_EQ(doc[2].data, "fub \\\"blah\\\" bar");
});

TEST_CASE(JSONTest, TestJSONReencoding1, [] () {
  auto orig_str = R"({"customer":"xxx","docid":{"set_id":"p","item_id":"123"},"attributes":{"title~en":"12- blubb silver version","description~en":"925 blah blahb blah -- \"12\" represents 12 different chinese zodiac sign animals 鼠 Rat is symbol of Charm  those who born in 1948、1960、1972、1984 、1996 and 2008 are belonged to Rat  product description: it is a pendant with 2 faces 1 face is shinny well-polished silver surface the other face is matt look finishing with a raw feeling therefore, this big pendant can be wear in both sides to match your apparel!  the pendant is linked by a 100cm/ 40 inch long silver plated chain.  pendant size: 8.5cm x 7cm, about 15 gr.  the necklace would be received with our gift packing especially for rat @\"12\" series!!  ******************************************************  you are welcome to visit my web-site","category2":"999"}})";
  auto doc = json::parseJSON(orig_str);
  TestJSONObject obj;
  obj.data = json::toJSONString(doc);
  auto obj_json = json::toJSONString(obj);
  auto obj2 = json::fromJSON<TestJSONObject>(obj_json);
  EXPECT_EQ(obj2.data, orig_str);
});

TEST_CASE(JSONTest, TestJSONOutputStreamStringEncoding, [] () {
  Buffer json;
  json::JSONOutputStream jsons(BufferOutputStream::fromBuffer(&json));

  const char teststr[] ="b\"la\x00ht\x0cx\x1bxe\nxs\rx\tx\\xt";
  jsons.addString(std::string(teststr, sizeof(teststr) - 1));
  EXPECT_EQ(
      json.toString(),
      "\"b\\\"la\\u0000ht\\fx\\u001bxe\\nxs\\rx\\tx\\\\xt\"");
});
