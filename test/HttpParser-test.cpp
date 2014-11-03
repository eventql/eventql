#include <xzero/http/v1/HttpParser.h>
#include <xzero/http/HttpListener.h>
#include <xzero/http/HttpStatus.h>
#include <xzero/Buffer.h>
#include <vector>
#include <gtest/gtest.h>

using namespace xzero;
using namespace xzero::http1;

class HttpParserCallbacks : public xzero::HttpListener {  // {{{
 public:
  HttpParserCallbacks() = default;

  bool onMessageBegin(const BufferRef& method, const BufferRef& entity,
                      HttpVersion version) override;
  bool onMessageBegin(HttpVersion version, HttpStatus code,
                      const BufferRef& text) override;
  bool onMessageBegin() override;
  bool onMessageHeader(const BufferRef& name, const BufferRef& value) override;
  bool onMessageHeaderEnd() override;
  bool onMessageContent(const BufferRef& chunk) override;
  bool onMessageEnd() override;
  void onProtocolError(xzero::HttpStatus code, const std::string& msg) override;

 public:
  std::function<bool(const BufferRef& method, const BufferRef& entity,
                     HttpVersion version)> requestStart;
  std::function<bool(HttpVersion version, HttpStatus code,
                     const BufferRef& text)> responseStart;
  std::function<bool()> messageStart;
  std::function<bool(const BufferRef& name, const BufferRef& value)> header;
  std::function<bool()> headerEnd;
  std::function<bool(const BufferRef& chunk)> content;
  std::function<bool()> end;
  std::function<void(xzero::HttpStatus, const std::string&)> protocolError;
};

bool HttpParserCallbacks::onMessageBegin(const BufferRef& method,
                                         const BufferRef& entity,
                                         HttpVersion version) {
  if (requestStart)
    return requestStart(method, entity, version);
  else
    return true;
}

bool HttpParserCallbacks::onMessageBegin(HttpVersion version, HttpStatus code,
                                         const BufferRef& text) {
  if (responseStart)
    return responseStart(version, code, text);
  else
    return true;
}

bool HttpParserCallbacks::onMessageBegin() {
  if (messageStart)
    return messageStart();
  else
    return true;
}

bool HttpParserCallbacks::onMessageHeader(const BufferRef& name,
                                          const BufferRef& value) {
  if (header)
    return header(name, value);
  else
    return true;
}

bool HttpParserCallbacks::onMessageHeaderEnd() {
  if (headerEnd)
    return headerEnd();
  else
    return true;
}

bool HttpParserCallbacks::onMessageContent(const BufferRef& chunk) {
  if (content)
    return content(chunk);
  else
    return true;
}

bool HttpParserCallbacks::onMessageEnd() {
  if (end)
    return end();
  else
    return true;
}

void HttpParserCallbacks::onProtocolError(HttpStatus code,
                                          const std::string& msg) {
  if (protocolError)
    protocolError(code, msg);
}
// }}}
class HttpParserListener : public xzero::HttpListener {  // {{{
 public:
  HttpParserListener();

  bool onMessageBegin(const BufferRef& method, const BufferRef& entity,
                      HttpVersion version) override;
  bool onMessageBegin(HttpVersion version, HttpStatus code,
                      const BufferRef& text) override;
  bool onMessageBegin() override;
  bool onMessageHeader(const BufferRef& name, const BufferRef& value) override;
  bool onMessageHeaderEnd() override;
  bool onMessageContent(const BufferRef& chunk) override;
  bool onMessageEnd() override;
  void onProtocolError(xzero::HttpStatus code, const std::string& msg) override;

 public:
  std::string method;
  std::string entity;
  HttpVersion version;
  HttpStatus statusCode;
  std::string statusReason;
  std::vector<std::pair<std::string, std::string>> headers;
  Buffer body;
  xzero::HttpStatus errorCode;
  std::string errorMessage;

  bool messageBegin;
  bool headerEnd;
  bool messageEnd;
};

HttpParserListener::HttpParserListener()
    : method(),
      entity(),
      version(HttpVersion::UNKNOWN),
      statusCode(HttpStatus::Undefined),
      statusReason(),
      headers(),
      errorCode(xzero::HttpStatus::Undefined),
      errorMessage(),
      messageBegin(false),
      headerEnd(false),
      messageEnd(false) {}

bool HttpParserListener::onMessageBegin(const BufferRef& method,
                                        const BufferRef& entity,
                                        HttpVersion version) {
  this->method = method.str();
  this->entity = entity.str();
  this->version = version;

  return true;
}

bool HttpParserListener::onMessageBegin(HttpVersion version, HttpStatus code,
                                        const BufferRef& text) {
  this->version = version;
  this->statusCode = code;
  this->statusReason = text.str();

  return true;
}

bool HttpParserListener::onMessageBegin() {
  messageBegin = true;
  return true;
}

bool HttpParserListener::onMessageHeader(const BufferRef& name,
                                         const BufferRef& value) {
  headers.push_back(std::make_pair(name.str(), value.str()));
  return true;
}

bool HttpParserListener::onMessageHeaderEnd() {
  headerEnd = true;
  return true;
}

bool HttpParserListener::onMessageContent(const BufferRef& chunk) {
  body += chunk;
  return true;
}

bool HttpParserListener::onMessageEnd() {
  messageEnd = true;
  return true;
}

void HttpParserListener::onProtocolError(xzero::HttpStatus code,
                                         const std::string& msg) {
  errorCode = code;
  errorMessage = msg;
}
// }}}

TEST(HttpParser, requestLine0) {
  /* Seems like in HTTP/0.9 it was possible to create
   * very simple request messages.
   */
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment("GET /\r\n");

  ASSERT_EQ("GET", listener.method);
  ASSERT_EQ("/", listener.entity);
  ASSERT_EQ(HttpVersion::VERSION_0_9, listener.version);
  ASSERT_TRUE(listener.headerEnd);
  ASSERT_TRUE(listener.messageEnd);
  ASSERT_EQ(0, listener.headers.size());
  ASSERT_EQ(0, listener.body.size());
}

TEST(HttpParser, requestLine1) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment("GET / HTTP/0.9\r\n\r\n");

  ASSERT_EQ("GET", listener.method);
  ASSERT_EQ("/", listener.entity);
  ASSERT_EQ(HttpVersion::VERSION_0_9, listener.version);
  ASSERT_EQ(0, listener.headers.size());
  ASSERT_EQ(0, listener.body.size());
}

TEST(HttpParser, requestLine2) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment("HEAD /foo?bar HTTP/1.0\r\n\r\n");

  ASSERT_EQ("HEAD", listener.method);
  ASSERT_EQ("/foo?bar", listener.entity);
  ASSERT_EQ(HttpVersion::VERSION_1_0, listener.version);
  ASSERT_EQ(0, listener.headers.size());
  ASSERT_EQ(0, listener.body.size());
}

TEST(HttpParser, requestLine_invalid1_MissingPathAndProtoVersion) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment("GET\r\n\r\n");
  ASSERT_EQ(HttpStatus::BadRequest, listener.errorCode);
}

TEST(HttpParser, requestLine_invalid3_InvalidVersion) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment("GET / HTTP/0\r\n\r\n");
  ASSERT_EQ((int)HttpStatus::BadRequest, (int)listener.errorCode);
}

TEST(HttpParser, requestLine_invalid3_CharsAfterVersion) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment("GET / HTTP/1.1b\r\n\r\n");
  ASSERT_EQ((int)HttpStatus::BadRequest, (int)listener.errorCode);
}

TEST(HttpParser, requestLine_invalid5_SpaceAfterVersion) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment("GET / HTTP/1.1 \r\n\r\n");
  ASSERT_EQ((int)HttpStatus::BadRequest, (int)listener.errorCode);
}

TEST(HttpParser, requestLine_invalid6_UnsupportedVersion) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment("GET / HTTP/1.2\r\n\r\n");
  ASSERT_EQ((int)HttpStatus::HttpVersionNotSupported, (int)listener.errorCode);
}

TEST(HttpParser, headers1) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::MESSAGE, &listener);
  parser.parseFragment(
      "Foo: the foo\r\n"
      "Content-Length: 6\r\n"
      "\r\n"
      "123456");

  ASSERT_EQ("Foo", listener.headers[0].first);
  ASSERT_EQ("the foo", listener.headers[0].second);
  ASSERT_EQ("123456", listener.body);
}

TEST(HttpParser, invalidHeader1) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::MESSAGE, &listener);
  size_t n = parser.parseFragment("Foo : the foo\r\n"
                                  "\r\n");

  ASSERT_EQ(HttpStatus::BadRequest, listener.errorCode);
  ASSERT_EQ(3, n);
  ASSERT_EQ(0, listener.headers.size());
}

TEST(HttpParser, invalidHeader2) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::MESSAGE, &listener);
  size_t n = parser.parseFragment("Foo\r\n"
                                  "\r\n");

  ASSERT_EQ(HttpStatus::BadRequest, listener.errorCode);
  ASSERT_EQ(5, n);
  ASSERT_EQ(0, listener.headers.size());
}

TEST(HttpParser, requestWithHeaders) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
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

TEST(HttpParser, requestWithHeadersAndBody) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
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
TEST(HttpParser, requestWithHeadersAndBodyChunked1) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment(
      "GET / HTTP/0.9\r\n"
      "Transfer-Encoding: chunked\r\n"
      "\r\n"
      "0\r\n"
      "\r\n");

  ASSERT_EQ("", listener.body);
}

// exactly one data chunk
TEST(HttpParser, requestWithHeadersAndBodyChunked2) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
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
TEST(HttpParser, requestWithHeadersAndBodyChunked3) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
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
TEST(HttpParser, requestWithHeadersAndBodyChunked_invalid1) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
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
  ASSERT_EQ(xzero::HttpStatus::BadRequest, listener.errorCode);
}

TEST(HttpParser, pipelined1) {
  HttpParserListener listener;
  HttpParser parser(HttpParser::REQUEST, &listener);
  parser.parseFragment("GET /foo HTTP/1.1\r\n\r\n"
                       "HEAD /bar HTTP/0.9\r\n\r\n");

  ASSERT_EQ("HEAD", listener.method);
  ASSERT_EQ("/bar", listener.entity);
  ASSERT_EQ(HttpVersion::VERSION_0_9, listener.version);
}

