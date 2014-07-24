// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/HttpClient.h>
#include <base/Socket.h>
#include <base/SocketSpec.h>
#include <fcntl.h>

namespace xzero {

using namespace base;

HttpClient::HttpClient(ev::loop_ref loop, const IPAddress& ipaddr, int port)
    : HttpMessageParser(HttpMessageParser::RESPONSE),
      loop_(loop),
      ipaddr_(ipaddr),
      port_(port),
      socket_(new Socket(loop)),
      writeBuffer_(),
      writeOffset_(0),
      readBuffer_(),
      readOffset_(),
      processingDone_(false),
      statusCode_(0),
      statusText_(),
      headers_() {}

HttpClient::~HttpClient() { delete socket_; }

void HttpClient::setRequest(const std::string& method, const std::string& path,
                            const HeaderMap& headers, const Buffer& content) {
  // initialize new request
  writeBuffer_.push_back(method);
  writeBuffer_.push_back(" ");
  writeBuffer_.push_back(path);
  writeBuffer_.push_back(" HTTP/1.1\r\n");

  for (const auto& header : headers) {
    writeBuffer_.push_back(header.first);
    writeBuffer_.push_back(": ");
    writeBuffer_.push_back(header.second);
    writeBuffer_.push_back("\r\n");
  }

  writeBuffer_.push_back("Connection: close\r\n");

  if (!content.empty()) {
    writeBuffer_.push_back(
        "Content-Length: "
        "\r\n");
    writeBuffer_.push_back(content.size());
    writeBuffer_.push_back("\r\n");
  }

  writeBuffer_.push_back("\r\n");
  writeBuffer_.push_back(content);
}

void HttpClient::start() {
  // reset state
  writeOffset_ = 0;
  readOffset_ = 0;
  readBuffer_.clear();
  statusText_.clear();
  statusCode_ = 0;
  headers_.clear();
  content_.clear();
  processingDone_ = false;

  if (socket_->isOpen()) socket_->close();

  socket_->open(SocketSpec::fromInet(IPAddress(ipaddr_), port_),
                O_CLOEXEC | O_NONBLOCK);
  if (!socket_->isOpen()) {
    reportError(HttpClientError::ConnectError);
  } else if (socket_->state() == Socket::Connecting) {
    socket_->setReadyCallback<HttpClient, &HttpClient::onConnectDone>(this);
    socket_->setMode(Socket::ReadWrite);
  } else {
    socket_->setReadyCallback<HttpClient, &HttpClient::io>(this);
    socket_->setMode(Socket::ReadWrite);
  }
}

void HttpClient::stop() {
  socket_->close();
  loop_.break_loop();
}

void HttpClient::reportError(HttpClientError ec) {
  responseHandler_(ec, 0, HeaderMap(), BufferRef());
}

void HttpClient::onConnectDone(Socket*, int revents) {
  if (socket_->state() == Socket::Operational) {
    socket_->setReadyCallback<HttpClient, &HttpClient::io>(this);
    socket_->setMode(Socket::ReadWrite);
  } else {
    reportError(HttpClientError::ConnectError);
  }
}

void HttpClient::io(Socket*, int revents) {
  if (revents & Socket::Write) writeSome();

  if (revents & Socket::Read) readSome();
}

void HttpClient::writeSome() {
  ssize_t rv = socket_->write(writeBuffer_.data() + writeOffset_,
                              writeBuffer_.size() - writeOffset_);

  if (rv < 0) {
    perror("socket.write: ");
    socket_->setMode(Socket::None);
    socket_->close();
    return;
    // TODO report & try again in %d seconds
  }

  writeOffset_ += rv;

  if (writeOffset_ == writeBuffer_.size()) {
    // request fully sent.
    socket_->setMode(Socket::Read);
  }
}

void HttpClient::readSome() {
  size_t lower_bound = readBuffer_.size();
  if (lower_bound == readBuffer_.capacity())
    readBuffer_.setCapacity(lower_bound + 8 * 1024);

  ssize_t rv = socket_->read(readBuffer_);

  if (rv > 0) {
    parseFragment(readBuffer_.ref(lower_bound, rv));

    if (state() == HttpMessageParser::PROTOCOL_ERROR) {
      responseHandler_(HttpClientError::ProtocolError, statusCode_, headers_,
                       content_.ref());
      stop();
    } else if (processingDone_) {
      responseHandler_(HttpClientError::Success, statusCode_, headers_,
                       content_.ref());
      stop();
    } else {
      socket_->setMode(Socket::Read);
    }
  } else if (rv != 0) {
    stop();
  } else {
    stop();
    if (state() == HttpMessageParser::CONTENT ||
        state() == HttpMessageParser::CONTENT_ENDLESS ||
        state() == HttpMessageParser::MESSAGE_BEGIN)
      responseHandler_(HttpClientError::Success, statusCode_, headers_,
                       content_.ref());
    else {
      responseHandler_(HttpClientError::ProtocolError, statusCode_, headers_,
                       content_.ref());
    }
  }
}

bool HttpClient::onMessageBegin(int versionMajor, int versionMinor, int code,
                                const BufferRef& text) {
  statusCode_ = code;
  statusText_ = text;
  return true;
}

bool HttpClient::onMessageHeader(const BufferRef& name,
                                 const BufferRef& value) {
  headers_[name.str()] = value.str();
  return true;
}

bool HttpClient::onMessageHeaderEnd() { return true; }

bool HttpClient::onMessageContent(const BufferRef& chunk) {
  content_.push_back(chunk);
  return true;
}

bool HttpClient::onMessageEnd() {
  processingDone_ = true;
  return true;
}

void HttpClient::log(LogMessage&& msg) {
  Buffer text;
  text << msg;
  fprintf(stderr, "HttpClient: %s\n", text.c_str());
}

void HttpClient::request(
    const IPAddress& host, int port, const std::string& method,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& headers,
    const Buffer& content, ResponseHandler callback) {
  ev::loop_ref loop(ev::default_loop(0));
  HttpClient cli(loop, host, port);
  cli.setRequest(method, path, headers, content);
  cli.setResultCallback(callback);
  cli.start();

  loop.run();
}

}  // namespace xzero
