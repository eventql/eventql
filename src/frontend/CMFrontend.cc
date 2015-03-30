/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-base/exception.h>
#include <fnord-base/inspect.h>
#include "fnord-base/logging.h"
#include <fnord-base/wallclock.h>
#include <fnord-http/cookies.h>
#include "fnord-http/httprequest.h"
#include "fnord-http/httpresponse.h"
#include "fnord-http/status.h"
#include "fnord-base/random.h"
#include "fnord-json/json.h"
#include "fnord-json/jsonrpcrequest.h"
#include "fnord-json/jsonrpcresponse.h"
#include "CustomerNamespace.h"
#include "IndexChangeRequest.h"
#include "frontend/CMFrontend.h"

using namespace fnord;

namespace cm {

const unsigned char pixel_gif[42] = {
  0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x21, 0xf9, 0x04, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
  0x00, 0x02, 0x01, 0x44, 0x00, 0x3b
};

CMFrontend::CMFrontend(
    feeds::RemoteFeedWriter* tracker_log_feed) :
    tracker_log_feed_(tracker_log_feed) {
  //pixel_log_feed_ = feed_factory->getFeed("cm.tracker.log");
  exportStats("/cm-frontend/global");
  exportStats(StringUtil::format("/cm-frontend/by-host/$0", cmHostname()));
}

void CMFrontend::exportStats(const std::string& prefix) {
  exportStat(
      StringUtil::format("$0/$1", prefix, "rpc_requests_total"),
      &stat_rpc_requests_total_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "rpc_errors_total"),
      &stat_rpc_errors_total_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "loglines_total"),
      &stat_loglines_total_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "loglines_versiontooold"),
      &stat_loglines_versiontooold_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "loglines_invalid"),
      &stat_loglines_invalid_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "loglines_written_success"),
      &stat_loglines_written_success_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "loglines_written_failure"),
      &stat_loglines_written_failure_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "index_requests_total"),
      &stat_index_requests_total_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "index_requests_written_success"),
      &stat_index_requests_written_success_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/$1", prefix, "index_requests_written_failure"),
      &stat_index_requests_written_failure_,
      fnord::stats::ExportMode::EXPORT_DELTA);
}

void CMFrontend::handleHTTPRequest(
    fnord::http::HTTPRequest* request,
    fnord::http::HTTPResponse* response) {
  /* find namespace */
  CustomerNamespace* ns = nullptr;
  const auto hostname = request->getHeader("host");

  auto ns_iter = vhosts_.find(hostname);
  if (ns_iter == vhosts_.end()) {
    response->setStatus(fnord::http::kStatusNotFound);
    response->addBody("not found");
    return;
  } else {
    ns = ns_iter->second;
  }

  fnord::URI uri(request->uri());

  if (uri.path() == "/t.js") {
    response->setStatus(fnord::http::kStatusOK);
    response->addHeader("Content-Type", "application/javascript");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    response->addBody(ns->trackingJS());
    return;
  }

  if (uri.path() == "/t.gif") {
    try {
      recordLogLine(ns, uri.query());
    } catch (const std::exception& e) {
      auto msg = fnord::StringUtil::format(
          "invalid tracking pixel url: $0",
          uri.query());

      fnord::logDebug("cm.frontend", e, msg);
    }

    response->setStatus(fnord::http::kStatusOK);
    response->addHeader("Content-Type", "image/gif");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    response->addBody((void *) &pixel_gif, sizeof(pixel_gif));
    return;
  }

  if (uri.path() == "/rpc") {
    stat_rpc_requests_total_.incr(1);
    response->setStatus(http::kStatusOK);
    json::JSONRPCResponse res(response->getBodyOutputStream());

    if (request->method() != http::HTTPRequest::M_POST) {
      res.error(
          json::JSONRPCResponse::kJSONRPCPInvalidRequestError,
          "HTTP method must be POST");
      return;
    }

    try {
      json::JSONRPCRequest req(json::parseJSON(request->body()));
      res.setID(req.id());
      dispatchRPC(&req, &res);
    } catch (const fnord::Exception& e) {
      stat_rpc_errors_total_.incr(1);
      res.error(
          json::JSONRPCResponse::kJSONRPCPInternalError,
          e.getMessage());
    }

    return;
  }

  response->setStatus(fnord::http::kStatusNotFound);
  response->addBody("not found");
}


void CMFrontend::dispatchRPC(
    json::JSONRPCRequest* req,
    json::JSONRPCResponse* res) {
  if (req->method() == "index_document") {
    auto index_req = req->getArg<IndexChangeRequest>(0, "index_request");
    recordIndexChangeRequest(index_req);

    res->success([] (json::JSONOutputStream* jos) {
      jos->addTrue();
    });

    return;
  }

  stat_rpc_errors_total_.incr(1);
  res->error(
      json::JSONRPCResponse::kJSONRPCPMethodNotFoundError,
      "invalid method");
}

void CMFrontend::addCustomer(
    CustomerNamespace* customer,
    RefPtr<feeds::RemoteFeedWriter> index_request_feed_writer) {
  for (const auto& vhost : customer->vhosts()) {
    if (vhosts_.count(vhost) != 0) {
      RAISEF(kRuntimeError, "hostname is already registered: $0", vhost);
    }

    vhosts_[vhost] = customer;
  }

  index_request_feeds_.emplace(customer->key(), index_request_feed_writer);
}

void CMFrontend::recordLogLine(
    CustomerNamespace* customer,
    const std::string& logline) {
  fnord::URI::ParamList params;
  fnord::URI::parseQueryString(logline, &params);

  stat_loglines_total_.incr(1);

  std::string pixel_ver;
  if (!fnord::URI::getParam(params, "v", &pixel_ver)) {
    stat_loglines_invalid_.incr(1);
    RAISE(kRuntimeError, "missing v parameter");
  }

  try {
    if (std::stoi(pixel_ver) < kMinPixelVersion) {
      stat_loglines_versiontooold_.incr(1);
      stat_loglines_invalid_.incr(1);
      RAISEF(kRuntimeError, "pixel version too old: $0", pixel_ver);
    }
  } catch (const std::exception& e) {
    stat_loglines_invalid_.incr(1);
    RAISEF(kRuntimeError, "invalid pixel version: $0", pixel_ver);
  }

  auto feedline = fnord::StringUtil::format(
      "$0|$1|$2",
      customer->key(),
      fnord::WallClock::unixSeconds(),
      logline);

  tracker_log_feed_->appendEntry(feedline);
}

void CMFrontend::recordIndexChangeRequest(const IndexChangeRequest& index_request) {
  stat_index_requests_total_.incr(1);

  auto ir_feed_iter = index_request_feeds_.find(index_request.customer);
  if (ir_feed_iter == index_request_feeds_.end()) {
    RAISEF(kIndexError, "unknown customer: $0", index_request.customer);
  }

  ir_feed_iter->second->appendEntry(json::toJSONString(index_request));
}

} // namespace cm
