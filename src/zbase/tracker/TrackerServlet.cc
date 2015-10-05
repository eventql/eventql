/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/exception.h>
#include <stx/inspect.h>
#include "stx/logging.h"
#include <stx/wallclock.h>
#include <stx/assets.h>
#include <stx/http/cookies.h>
#include "stx/http/httprequest.h"
#include "stx/http/httpresponse.h"
#include "stx/http/status.h"
#include "stx/random.h"
#include "stx/json/json.h"
#include "stx/json/jsonrpcrequest.h"
#include "stx/json/jsonrpcresponse.h"
#include "zbase/tracker/TrackerServlet.h"
#include "zbase/tracker/ztrackid.h"

using namespace stx;

namespace zbase {

const unsigned char pixel_gif[42] = {
  0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x21, 0xf9, 0x04, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
  0x00, 0x02, 0x01, 0x44, 0x00, 0x3b
};

TrackerServlet::TrackerServlet(
    feeds::RemoteFeedWriter* tracker_log_feed) :
    tracker_log_feed_(tracker_log_feed) {
  exportStats("/ztracker/global");
  //exportStats(StringUtil::format("/ztracker/by-host/$0", cmHostname()));
}

void TrackerServlet::exportStats(const std::string& prefix) {
  exportStat(
      StringUtil::format("$0/$1", prefix, "rpc_requests_total"),
      &stat_rpc_requests_total_,
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "rpc_errors_total"),
      &stat_rpc_errors_total_,
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "loglines_total"),
      &stat_loglines_total_,
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "loglines_versiontooold"),
      &stat_loglines_versiontooold_,
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "loglines_invalid"),
      &stat_loglines_invalid_,
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "loglines_written_success"),
      &stat_loglines_written_success_,
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "loglines_written_failure"),
      &stat_loglines_written_failure_,
      stx::stats::ExportMode::EXPORT_DELTA);
}

void TrackerServlet::handleHTTPRequest(
    stx::http::HTTPRequest* request,
    stx::http::HTTPResponse* response) {
  stx::URI uri(request->uri());

  response->addHeader("Access-Control-Allow-Origin", "*"); // FIXME
  response->addHeader("Access-Control-Allow-Methods", "GET, POST");

  if (request->method() == http::HTTPMessage::M_OPTIONS) {
    response->setStatus(http::kStatusOK);
    return;
  }

  static const String kJSAPIPrefix = "/api-";
  static const String kJSAPISuffix = ".js";
  if (StringUtil::beginsWith(uri.path(), kJSAPIPrefix) &&
      StringUtil::endsWith(uri.path(), kJSAPISuffix)) {
    auto ztrackid = uri.path().substr(
        kJSAPIPrefix.size(),
        uri.path().size() - kJSAPIPrefix.size() - kJSAPISuffix.size());
    auto customer = ztrackid_decode(ztrackid);
    iputs("get pixel for $0 => $1", ztrackid, customer);

    response->setStatus(stx::http::kStatusOK);
    response->addHeader("Content-Type", "application/javascript");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    response->addBody(Assets::getAsset("zbase/tracker/track.js"));
    return;
  }

  static const String kPushAPIPrefix = "/push-";
  if (StringUtil::beginsWith(uri.path(), kPushAPIPrefix)) {
    auto ztrackid = uri.path().substr(
        kPushAPIPrefix.size(),
        uri.path().size() - kPushAPIPrefix.size());
    auto customer = ztrackid_decode(ztrackid);

    String pixeldata;
    if (request->method() == http::HTTPMessage::M_POST) {
      pixeldata = request->body().toString();
    } else {
      pixeldata = uri.query();
    }

    try {
      pushEvent(customer, pixeldata);
    } catch (const std::exception& e) {
      auto msg = stx::StringUtil::format(
          "invalid tracking pixel data: $0",
          pixeldata);

      stx::logDebug("cm.frontend", e, msg);
    }

    response->setStatus(stx::http::kStatusOK);
    response->addHeader("Content-Type", "image/gif");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    response->addBody((void *) &pixel_gif, sizeof(pixel_gif));
    return;
  }

  if (uri.path() == "/ping") {
    response->setStatus(http::kStatusOK);
    response->addBody("pong");
    return;
  }

  response->setStatus(stx::http::kStatusNotFound);
  response->addBody("not found");
}

void TrackerServlet::pushEvent(
    const String& customer,
    const std::string& ev) {
  iputs("incoming logline: $0 => $1", customer, ev);

  stat_loglines_total_.incr(1);

  //stx::URI::ParamList params;
  //stx::URI::parseQueryString(logline, &params);

  //std::string pixel_ver;
  //if (!stx::URI::getParam(params, "v", &pixel_ver)) {
  //  stat_loglines_invalid_.incr(1);
  //  RAISE(kRuntimeError, "missing v parameter");
  //}

  //try {
  //  if (std::stoi(pixel_ver) < kMinPixelVersion) {
  //    stat_loglines_versiontooold_.incr(1);
  //    stat_loglines_invalid_.incr(1);
  //    RAISEF(kRuntimeError, "pixel version too old: $0", pixel_ver);
  //  }
  //} catch (const std::exception& e) {
  //  stat_loglines_invalid_.incr(1);
  //  RAISEF(kRuntimeError, "invalid pixel version: $0", pixel_ver);
  //}

  auto feedline = stx::StringUtil::format(
     "$0|$1|1|$2",
     customer,
     stx::WallClock::unixSeconds(),
     ev);

  tracker_log_feed_->appendEntry(feedline);
}

} // namespace zbase
