/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <algorithm>
#include <stx/inspect.h>
#include <stx/http/httprequest.h>

using stx::InputStream;
using stx::OutputStream;
using stx::StringInputStream;
using stx::StringOutputStream;

namespace stx {
namespace http {

std::string HTTPMessage::kEmptyHeader = "";

const std::string& HTTPMessage::version() const {
  return version_;
}

void HTTPMessage::setVersion(const std::string& version) {
  version_ = version;
}

const std::vector<std::pair<std::string, std::string>>&
    HTTPMessage::headers() const {
  return headers_;
}

const std::string& HTTPMessage::getHeader(const std::string& key) const {
  auto key_low = key;
  std::transform(key_low.begin(), key_low.end(), key_low.begin(), ::tolower);

  for (const auto& header : headers_) {
    if (header.first == key_low) {
      return header.second;
    }
  }

  return kEmptyHeader;
}

bool HTTPMessage::hasHeader(const std::string& key) const {
  auto key_low = key;
  std::transform(key_low.begin(), key_low.end(), key_low.begin(), ::tolower);

  for (const auto& header : headers_) {
    if (header.first == key_low) {
      return true;
    }
  }

  return false;
}

void HTTPMessage::addHeader(const std::string& key, const std::string& value) {
  auto key_low = key;
  std::transform(key_low.begin(), key_low.end(), key_low.begin(), ::tolower);
  headers_.emplace_back(key_low, value);
}

void HTTPMessage::setHeader(const std::string& key, const std::string& value) {
  auto key_low = key;
  std::transform(key_low.begin(), key_low.end(), key_low.begin(), ::tolower);

  for (auto& header : headers_) {
    if (header.first == key_low) {
      header.second = value;
      return;
    }
  }

  headers_.emplace_back(key_low, value);
}

void HTTPMessage::clearHeaders() {
  headers_.clear();
}

const Buffer& HTTPMessage::body() const {
  return body_;
}

void HTTPMessage::addBody(const std::string& body) {
  body_ = Buffer(body.c_str(), body.length());
}

void HTTPMessage::addBody(const void* data, size_t size) {
  body_ = Buffer(data, size);
}

void HTTPMessage::addBody(const Buffer& buf) {
  body_ = buf;
}

void HTTPMessage::appendBody(const void* data, size_t size) {
  body_.append(data, size);
}

void HTTPMessage::clearBody() {
  body_.clear();
}

std::unique_ptr<InputStream> HTTPMessage::getBodyInputStream() const {
  return BufferInputStream::fromBuffer(&body_);
}

std::unique_ptr<OutputStream> HTTPMessage::getBodyOutputStream() {
  return BufferOutputStream::fromBuffer(&body_);
}

}

template <>
std::string StringUtil::toString(http::HTTPMessage::kHTTPMethod method) {
  switch (method) {
    case http::HTTPMessage::M_CONNECT: return "CONNECT";
    case http::HTTPMessage::M_DELETE: return "DELETE";
    case http::HTTPMessage::M_GET: return "GET";
    case http::HTTPMessage::M_HEAD: return "HEAD";
    case http::HTTPMessage::M_OPTIONS: return "OPTIONS";
    case http::HTTPMessage::M_POST: return "POST";
    case http::HTTPMessage::M_PUT: return "PUT";
    case http::HTTPMessage::M_TRACE: return "TRACE";
    default: RAISE(kTypeError, "invalid HTTP method");
  }
}

template <>
std::string inspect(const http::HTTPMessage::kHTTPMethod& method) {
  return StringUtil::toString(method);
}

}
