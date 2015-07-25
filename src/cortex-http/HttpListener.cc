// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <cortex-http/HttpListener.h>

namespace cortex {
namespace http {

void HttpListener::onMessageBegin(const BufferRef& method,
                                  const BufferRef& uri,
                                  HttpVersion version) {
}

void HttpListener::onMessageBegin(HttpVersion version, HttpStatus code,
                                  const BufferRef& text) {
}

void HttpListener::onMessageBegin() {
}

void HttpListener::onMessageHeader(const BufferRef& name,
                                   const BufferRef& value) {
}

void HttpListener::onMessageHeaderEnd() {
}

void HttpListener::onMessageContent(const BufferRef& chunk) {
}

void HttpListener::onMessageEnd() {
}

void HttpListener::onProtocolError(HttpStatus code,
                                   const std::string& message) {
}

}  // namespace http
}  // namespace cortex
