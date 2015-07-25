// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-http/http1/Parser.h>
#include <cortex-http/HttpListener.h>
#include <cortex-http/HttpStatus.h>
#include <cortex-base/RuntimeError.h>
#include <cortex-base/Buffer.h>
#include <vector>
#include <gtest/gtest.h>

using namespace cortex;
using namespace cortex::http;
using namespace cortex::http::http1;

class ParserListener : public HttpListener {  // {{{
 public:
  ParserListener();

  void onMessageBegin(const BufferRef& method, const BufferRef& entity,
                      HttpVersion version) override;
  void onMessageBegin(HttpVersion version, HttpStatus code,
                      const BufferRef& text) override;
  void onMessageBegin() override;
  void onMessageHeader(const BufferRef& name, const BufferRef& value) override;
  void onMessageHeaderEnd() override;
  void onMessageContent(const BufferRef& chunk) override;
  void onMessageEnd() override;
  void onProtocolError(HttpStatus code, const std::string& msg) override;

 public:
  std::string method;
  std::string entity;
  HttpVersion version;
  HttpStatus statusCode;
  std::string statusReason;
  std::vector<std::pair<std::string, std::string>> headers;
  Buffer body;
  HttpStatus errorCode;
  std::string errorMessage;

  bool messageBegin;
  bool headerEnd;
  bool messageEnd;
};

ParserListener::ParserListener()
    : method(),
      entity(),
      version(HttpVersion::UNKNOWN),
      statusCode(HttpStatus::Undefined),
      statusReason(),
      headers(),
      errorCode(HttpStatus::Undefined),
      errorMessage(),
      messageBegin(false),
      headerEnd(false),
      messageEnd(false) {
}

void ParserListener::onMessageBegin(const BufferRef& method,
                                        const BufferRef& entity,
                                        HttpVersion version) {
  this->method = method.str();
  this->entity = entity.str();
  this->version = version;
}

void ParserListener::onMessageBegin(HttpVersion version, HttpStatus code,
                                        const BufferRef& text) {
  this->version = version;
  this->statusCode = code;
  this->statusReason = text.str();
}

void ParserListener::onMessageBegin() {
  messageBegin = true;
}

void ParserListener::onMessageHeader(const BufferRef& name,
                                         const BufferRef& value) {
  headers.push_back(std::make_pair(name.str(), value.str()));
}

void ParserListener::onMessageHeaderEnd() {
  headerEnd = true;
}

void ParserListener::onMessageContent(const BufferRef& chunk) {
  body += chunk;
}

void ParserListener::onMessageEnd() {
  messageEnd = true;
}

void ParserListener::onProtocolError(HttpStatus code,
                                         const std::string& msg) {
  errorCode = code;
  errorMessage = msg;
}
// }}}

TEST(http_http1_Parser, requestLine0) {
  /* Seems like in HTTP/0.9 it was possible to create
   * very simple request messages.
   */
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  parser.parseFragment("GET /\r\n");

  ASSERT_EQ("GET", listener.method);
  ASSERT_EQ("/", listener.entity);
  ASSERT_EQ(HttpVersion::VERSION_0_9, listener.version);
  ASSERT_TRUE(listener.headerEnd);
  ASSERT_TRUE(listener.messageEnd);
  ASSERT_EQ(0, listener.headers.size());
  ASSERT_EQ(0, listener.body.size());
}

TEST(http_http1_Parser, requestLine1) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  parser.parseFragment("GET / HTTP/0.9\r\n\r\n");

  ASSERT_EQ("GET", listener.method);
  ASSERT_EQ("/", listener.entity);
  ASSERT_EQ(HttpVersion::VERSION_0_9, listener.version);
  ASSERT_EQ(0, listener.headers.size());
  ASSERT_EQ(0, listener.body.size());
}

TEST(http_http1_Parser, requestLine2) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  parser.parseFragment("HEAD /foo?bar HTTP/1.0\r\n\r\n");

  ASSERT_EQ("HEAD", listener.method);
  ASSERT_EQ("/foo?bar", listener.entity);
  ASSERT_EQ(HttpVersion::VERSION_1_0, listener.version);
  ASSERT_EQ(0, listener.headers.size());
  ASSERT_EQ(0, listener.body.size());
}

TEST(http_http1_Parser, requestLine_invalid1_MissingPathAndProtoVersion) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  parser.parseFragment("GET\r\n\r\n");
  ASSERT_EQ(HttpStatus::BadRequest, listener.errorCode);
}

TEST(http_http1_Parser, requestLine_invalid3_InvalidVersion) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  parser.parseFragment("GET / HTTP/0\r\n\r\n");
  ASSERT_EQ((int)HttpStatus::BadRequest, (int)listener.errorCode);
}

TEST(http_http1_Parser, requestLine_invalid3_CharsAfterVersion) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  parser.parseFragment("GET / HTTP/1.1b\r\n\r\n");
  ASSERT_EQ((int)HttpStatus::BadRequest, (int)listener.errorCode);
}

TEST(http_http1_Parser, requestLine_invalid5_SpaceAfterVersion) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  parser.parseFragment("GET / HTTP/1.1 \r\n\r\n");
  ASSERT_EQ((int)HttpStatus::BadRequest, (int)listener.errorCode);
}

TEST(http_http1_Parser, requestLine_invalid6_UnsupportedVersion) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);

  // Actually, we could make it a ParserError, or HttpClientError or so,
  // But to make googletest lib happy, we should make it even a distinct class.
  ASSERT_THROW(parser.parseFragment("GET / HTTP/1.2\r\n\r\n"), RuntimeError);
}

TEST(http_http1_Parser, headers1) {
  ParserListener listener;
  Parser parser(Parser::MESSAGE, &listener);
  parser.parseFragment(
      "Foo: the foo\r\n"
      "Content-Length: 6\r\n"
      "\r\n"
      "123456");

  ASSERT_EQ("Foo", listener.headers[0].first);
  ASSERT_EQ("the foo", listener.headers[0].second);
  ASSERT_EQ("123456", listener.body);
}

TEST(http_http1_Parser, invalidHeader1) {
  ParserListener listener;
  Parser parser(Parser::MESSAGE, &listener);
  size_t n = parser.parseFragment("Foo : the foo\r\n"
                                  "\r\n");

  ASSERT_EQ(HttpStatus::BadRequest, listener.errorCode);
  ASSERT_EQ(3, n);
  ASSERT_EQ(0, listener.headers.size());
}

TEST(http_http1_Parser, invalidHeader2) {
  ParserListener listener;
  Parser parser(Parser::MESSAGE, &listener);
  size_t n = parser.parseFragment("Foo\r\n"
                                  "\r\n");

  ASSERT_EQ(HttpStatus::BadRequest, listener.errorCode);
  ASSERT_EQ(5, n);
  ASSERT_EQ(0, listener.headers.size());
}

TEST(http_http1_Parser, requestWithHeaders) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  parser.parseFragment(
      "GET / HTTP/0.9\r\n"
      "Foo: the foo\r\n"
      "X-Bar: the bar\r\n"
      "\r\n");

  ASSERT_EQ("GET", listener.method);
  ASSERT_EQ("/", listener.entity);
  ASSERT_EQ(HttpVersion::VERSION_0_9, listener.version);
  ASSERT_EQ(2, listener.headers.size());
  ASSERT_EQ(0, listener.body.size());

  ASSERT_EQ("Foo", listener.headers[0].first);
  ASSERT_EQ("the foo", listener.headers[0].second);

  ASSERT_EQ("X-Bar", listener.headers[1].first);
  ASSERT_EQ("the bar", listener.headers[1].second);
}

TEST(http_http1_Parser, requestWithHeadersAndBody) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  parser.parseFragment(
      "GET / HTTP/0.9\r\n"
      "Foo: the foo\r\n"
      "X-Bar: the bar\r\n"
      "Content-Length: 6\r\n"
      "\r\n"
      "123456");

  ASSERT_EQ("123456", listener.body);
}

// no chunks except the EOS-chunk
TEST(http_http1_Parser, requestWithHeadersAndBodyChunked1) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  parser.parseFragment(
      "GET / HTTP/0.9\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "0\r\n"
      "\r\n");

  ASSERT_EQ("", listener.body);
}

// exactly one data chunk
TEST(http_http1_Parser, requestWithHeadersAndBodyChunked2) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  parser.parseFragment(
      "GET / HTTP/0.9\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"

      "6\r\n"
      "123456"
      "\r\n"

      "0\r\n"
      "\r\n");

  ASSERT_EQ("123456", listener.body);
}

// more than one data chunk
TEST(http_http1_Parser, requestWithHeadersAndBodyChunked3) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  parser.parseFragment(
      "GET / HTTP/0.9\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"

      "6\r\n"
      "123456"
      "\r\n"

      "6\r\n"
      "123456"
      "\r\n"

      "0\r\n"
      "\r\n");

  ASSERT_EQ("123456123456", listener.body);
}

// first chunk is missing CR LR
TEST(http_http1_Parser, requestWithHeadersAndBodyChunked_invalid1) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  size_t n = parser.parseFragment(
      "GET / HTTP/0.9\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"

      "6\r\n"
      "123456"
      //"\r\n" // should bailout here

      "0\r\n"
      "\r\n");

  ASSERT_EQ(55, n);
  ASSERT_EQ(HttpStatus::BadRequest, listener.errorCode);
}

TEST(http_http1_Parser, pipelined1) {
  ParserListener listener;
  Parser parser(Parser::REQUEST, &listener);
  constexpr BufferRef input = "GET /foo HTTP/1.1\r\n\r\n"
                              "HEAD /bar HTTP/0.9\r\n\r\n";
  size_t n = parser.parseFragment(input);

  EXPECT_EQ("GET", listener.method);
  EXPECT_EQ("/foo", listener.entity);
  EXPECT_EQ(HttpVersion::VERSION_1_1, listener.version);

  parser.parseFragment(input.ref(n));

  EXPECT_EQ("HEAD", listener.method);
  EXPECT_EQ("/bar", listener.entity);
  EXPECT_EQ(HttpVersion::VERSION_0_9, listener.version);
}
