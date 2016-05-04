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

namespace stx {
namespace http {

struct HTTPSSEEvent {
  String data;
  Option<String> name;
};

class HTTPSSEParser {
public:

  void onEvent(Function<void (const HTTPSSEEvent& ev)> fn);

  void parse(const char* data, size_t size);

protected:

  void parseEvent(const char* data, size_t size);

  Buffer buf_;
  Function<void (const HTTPSSEEvent& ev)> on_event_;
};

}
}
