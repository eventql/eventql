/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _METRIC_SENSORSERVLET_H
#define _METRIC_SENSORSERVLET_H
#include "fnord/http/httpservice.h"
#include <fnord/random.h>
#include <sensord/SensorSampleFeed.h>

namespace fnord {
namespace metricdb {

class SensorPushServlet : public fnord::http::HTTPService {
public:

  SensorPushServlet(sensord::SensorSampleFeed* sensor_feed);

  void handleHTTPRequest(
      http::HTTPRequest* req,
      http::HTTPResponse* res);

protected:

  void pushSample(
      http::HTTPRequest* req,
      http::HTTPResponse* res,
      URI* uri);

  sensord::SensorSampleFeed* sensor_feed_;
};

}
}
#endif
