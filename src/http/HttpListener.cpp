// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <xzero/http/HttpListener.h>

namespace xzero {

bool HttpListener::onMessageBegin(const BufferRef& method, const BufferRef& uri,
                                  HttpVersion version) {
  return true;
}

bool HttpListener::onMessageBegin(HttpVersion version, HttpStatus code,
                                  const BufferRef& text) {
  return true;
}

bool HttpListener::onMessageBegin() {
  //.
  return true;
}

bool HttpListener::onMessageHeader(const BufferRef& name,
                                   const BufferRef& value) {
  return true;
}

bool HttpListener::onMessageHeaderEnd() {
  //.
  return true;
}

bool HttpListener::onMessageContent(const BufferRef& chunk) {
  //.
  return true;
}

bool HttpListener::onMessageEnd() {
  //.
  return true;
}

void HttpListener::onProtocolError(HttpStatus code,
                                   const std::string& message) {
  //.
}

}  // namespace xzero
