/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _FNORD_HTTP_HTTPSSESTREAM_H
#define _FNORD_HTTP_HTTPSSESTREAM_H
#include "fnord-http/httpservice.h"
#include "fnord-json/json.h"
#include <fnord-base/inspect.h>

namespace fnord {
namespace http {

class HTTPSSEStream {
public:

  HTTPSSEStream(
    RefPtr<http::HTTPRequestStream> req_stream,
    RefPtr<http::HTTPResponseStream> res_stream);

  void start();

  void sendEvent(
    const String& data,
    const Option<String>& event_type);

  void sendEvent(
    const Buffer& data,
    const Option<String>& event_type);

  void sendEvent(
    const void* event_data,
    size_t event_size,
    const Option<String>& event_type);

  const HTTPResponse response() const;
  void finish();

private:

  RefPtr<http::HTTPResponseStream> res_stream_;
  RefPtr<http::HTTPRequestStream> req_stream_;
  HTTPResponse res_;
};

}
}
#endif
