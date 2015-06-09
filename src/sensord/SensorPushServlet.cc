/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/wallclock.h>
#include <fnord-msg/msg.h>
#include <sensord/SensorPushServlet.h>

namespace fnord {
namespace metricdb {

SensorServlet::SensorServlet(
    sensord::SensorSampleFeed* sensor_feed) :
    sensor_feed_(sensor_feed) {}

void SensorServlet::handleHTTPRequest(
    fnord::http::HTTPRequest* req,
    fnord::http::HTTPResponse* res) {
  URI uri(req->uri());

  res->addHeader("Access-Control-Allow-Origin", "*");

  try {
    if (StringUtil::endsWith(uri.path(), "/push")) {
      return pushSample(req, res, &uri);
    }

    res->setStatus(fnord::http::kStatusNotFound);
    res->addBody("not found");
  } catch (const Exception& e) {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0: $1", e.getTypeName(), e.getMessage()));
  }
}

void SensorServlet::pushSample(
    http::HTTPRequest* req,
    http::HTTPResponse* res,
    URI* uri) {
  const auto& params = uri->queryParams();

  sensord::SampleList samples;
  msg::decode(req->body(), &samples);

  for (const auto& sample : samples.samples()) {
    sensor_feed_->publish(sample);
  }

  res->setStatus(http::kStatusCreated);
}

}
}

