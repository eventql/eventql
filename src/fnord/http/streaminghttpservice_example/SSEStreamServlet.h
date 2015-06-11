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
#ifndef _CM_SSESTREAMSERVLET_H
#define _CM_SSESTREAMSERVLET_H
#include <unistd.h>
#include "fnord-http/httpservice.h"
#include "fnord-http/HTTPSSEStream.h"

namespace fnord {
namespace http {

class SSEStreamServlet : public http::StreamingHTTPService {
public:

  void handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req,
      RefPtr<http::HTTPResponseStream> res);

};

}
}
#endif
