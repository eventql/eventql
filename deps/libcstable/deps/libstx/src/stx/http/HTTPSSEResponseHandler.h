/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/uri.h>
#include <stx/io/file.h>
#include <stx/http/httpmessage.h>
#include "stx/http/httprequest.h"
#include "stx/http/httpresponse.h"
#include "stx/http/httpstats.h"
#include "stx/http/httpconnectionpool.h"
#include "stx/http/httpclient.h"
#include "stx/http/HTTPSSEParser.h"

namespace stx {
namespace http {

class HTTPSSEResponseHandler : public HTTPResponseFuture {
public:
  typedef
      Function<HTTPResponseFuture* (const Promise<http::HTTPResponse>)>
      FactoryFn;

  typedef
      Function<void (const HTTPSSEEvent& ev)>
      CallbackFn;

  static FactoryFn getFactory(CallbackFn on_event);

protected:

  HTTPSSEResponseHandler(
      Promise<HTTPResponse> promise,
      CallbackFn on_event);

  void onBodyChunk(const char* data, size_t size) override;

  HTTPSSEParser parser_;
};

}
}
