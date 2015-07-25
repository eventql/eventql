// This file is part of the "libcortex" project
//   (c) 2009-2015 Christian Parpart <https://github.com/christianparpart>
//   (c) 2014-2015 Paul Asmuth <https://github.com/paulasmuth>
//
// libcortex is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License v3.0.
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <initializer_list>
#include <vector>
#include <gtest/gtest.h>
#include <cortex-http/fastcgi/RequestParser.h>
#include <cortex-http/fastcgi/ResponseParser.h>
#include <cortex-http/HttpRequest.h>
#include <cortex-http/HttpResponse.h>
#include <cortex-http/HttpChannel.h>

using namespace cortex;
using namespace cortex::http;

class ResponseListener : public HttpListener { // {{{
 public:
  void onMessageBegin(const BufferRef& method, const BufferRef& entity,
                      HttpVersion version) override;
  void onMessageBegin(HttpVersion version, HttpStatus code,
                      const BufferRef& text) override;
  void onMessageBegin() override;
  void onMessageHeader(const BufferRef& name, const BufferRef& value) override;
  void onMessageHeaderEnd() override;
  void onMessageContent(const BufferRef& chunk) override;
  void onMessageEnd() override;
  void onProtocolError(HttpStatus code, const std::string& message) override;

 public:
  BufferRef method;
  BufferRef entity;
  HttpVersion version = HttpVersion::VERSION_0_9;
  HttpStatus status = HttpStatus::Undefined;
  BufferRef text;
  int requestMessageBeginCount = 0;
  int genericMessageBeginCount = 0;
  int responseMessageBeginCount = 0;
  HeaderFieldList headers;
  int headersEnd = 0;
  int messageEnd = 0;
  int protocolErrors = 0;
  Buffer body;
};

void ResponseListener::onMessageBegin(const BufferRef& method, const BufferRef& entity,
                    HttpVersion version) {
  this->method = method;
  this->entity = entity;
  this->version = version;
  this->requestMessageBeginCount++;
}

void ResponseListener::onMessageBegin(HttpVersion version, HttpStatus code,
                    const BufferRef& text) {
  this->version = version;
  this->status = code;
  this->text = text;
  this->responseMessageBeginCount++;
}

void ResponseListener::onMessageBegin() {
  this->genericMessageBeginCount++;
}

void ResponseListener::onMessageHeader(const BufferRef& name, const BufferRef& value) {
  this->headers.push_back(name.str(), value.str());
}

void ResponseListener::onMessageHeaderEnd() {
  this->headersEnd++;
}

void ResponseListener::onMessageContent(const BufferRef& chunk) {
  this->body.push_back(chunk);
}

void ResponseListener::onMessageEnd() {
  this->messageEnd++;
}

void ResponseListener::onProtocolError(HttpStatus code, const std::string& message) {
  this->protocolErrors++;
}
// }}}

TEST(http_fastcgi_ResponseParser, simpleResponse) {
  // Tests the following example from the FastCGI spec:
  //
  // {FCGI_STDOUT,      1, "Content-type: text/html\r\n\r\n<html>\n<head> ... "}
  // {FCGI_STDOUT,      1, ""}
  // {FCGI_END_REQUEST, 1, {0, FCGI_REQUEST_COMPLETE}}

  // --------------------------------------------------------------------------
  // generate response bitstream

  constexpr BufferRef content =
      "Status: 200\r\n"
      "Content-Type: text/html\r\n"
      "\r\n"
      "<html>\n<head> ...";

  Buffer fcgistream;
  fcgistream.reserve(1024);

  // StdOut-header with payload
  new(fcgistream.data()) http::fastcgi::Record(
      http::fastcgi::Type::StdOut, 1, content.size(), 0);
  fcgistream.resize(sizeof(http::fastcgi::Record));
  fcgistream.push_back(content);

  // StdOut-header (EOS)
  new(fcgistream.end()) http::fastcgi::Record(
      http::fastcgi::Type::StdOut, 1, 0, 0);
  fcgistream.resize(fcgistream.size() + sizeof(http::fastcgi::Record));

  // EndRequest-header
  new(fcgistream.end()) http::fastcgi::EndRequestRecord(
      1, // requestId
      0, // appStatus
      http::fastcgi::ProtocolStatus::RequestComplete);
  fcgistream.resize(fcgistream.size() + sizeof(http::fastcgi::EndRequestRecord));

  // --------------------------------------------------------------------------
  // parse the response bitstream

  int parsedRequestId = -1;
  ResponseListener httpListener;
  auto onCreateChannel = [&](int requestId) -> HttpListener* {
                            parsedRequestId = requestId;
                            return &httpListener; };
  auto onUnknownPacket = [&](int requestId, int recordId) { };
  auto onStdErr = [&](int requestId, const BufferRef& content) { };

  http::fastcgi::ResponseParser parser(
      onCreateChannel, onUnknownPacket, onStdErr);

  size_t n = parser.parseFragment(fcgistream);

  EXPECT_EQ(fcgistream.size(), n);
  EXPECT_EQ(0, httpListener.protocolErrors);
  EXPECT_EQ(1, parsedRequestId);
  EXPECT_EQ("text/html", httpListener.headers.get("Content-Type"));
  EXPECT_EQ("<html>\n<head> ...", httpListener.body);
}
