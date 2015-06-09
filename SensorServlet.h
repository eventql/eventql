/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_BROKER_BROKERSERVLET_H
#define _FNORD_BROKER_BROKERSERVLET_H
#include "fnord-http/httpservice.h"
#include <fnord-base/random.h>
#include <fnord-feeds/FeedService.h>

namespace fnord {
namespace metricdb {

class SensorServlet : public fnord::http::HTTPService {
public:

  void handleHTTPRequest(
      http::HTTPRequest* req,
      http::HTTPResponse* res);

protected:

  void pushSample(
      http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

};

}
}
#endif
