/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/http/HTTPSSEResponseHandler.h"

namespace stx {
namespace http {

HTTPSSEResponseHandler::HTTPSSEResponseHandler(
    Promise<HTTPResponse> promise,
    CallbackFn on_event) :
    HTTPResponseFuture(promise) {
  parser_.onEvent(on_event);
}

HTTPSSEResponseHandler::FactoryFn HTTPSSEResponseHandler::getFactory(
    CallbackFn on_event) {
  return [on_event] (
      const Promise<http::HTTPResponse> promise) -> HTTPResponseFuture* {
    return new HTTPSSEResponseHandler(promise, on_event);
  };
}

void HTTPSSEResponseHandler::onBodyChunk(
    const char* data,
    size_t size) {
  parser_.parse(data, size);
}

}
}
