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
#include <eventql/util/uri.h>
#include <eventql/util/io/file.h>
#include <eventql/util/http/httpmessage.h>
#include "eventql/util/http/httprequest.h"
#include "eventql/util/http/httpresponse.h"
#include "eventql/util/http/httpstats.h"
#include "eventql/util/http/httpconnectionpool.h"
#include "eventql/util/http/httpclient.h"
#include "eventql/util/http/HTTPSSEParser.h"

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
