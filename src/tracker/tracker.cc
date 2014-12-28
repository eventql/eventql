/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "tracker/tracker.h"
#include <fnord/base/exception.h>
#include <fnord/base/inspect.h>
#include <fnord/net/http/cookies.h>
#include "fnord/net/http/httprequest.h"
#include "fnord/net/http/httpresponse.h"
#include "fnord/net/http/status.h"
#include "fnord/base/random.h"
#include "fnord/logging/logger.h"
#include "customernamespace.h"
#include "tracker/logjoinservice.h"


/**
 * mandatory params:
 *  c    -- clickid     -- format "<uid>~<eventid>", e.g. "f97650cb~b28c61d5c"
 *  e    -- eventtype   -- format "{q,v}" (query, visit)
 *
 * params for eventtype=q (query):
 *  is   -- item ids    -- format "<setid>~<itemid>~<pos>,..."
 *
 * params for eventtype=v (visit):
 *  i    -- itemid      -- format "<setid>~<itemid>"
 *
 */

namespace cm {

const unsigned char pixel_gif[42] = {
  0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x21, 0xf9, 0x04, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
  0x00, 0x02, 0x01, 0x44, 0x00, 0x3b
};

Tracker::Tracker(
    fnord::comm::FeedFactory* feed_factory) :
    feed_factory_(feed_factory) {}

bool Tracker::isReservedParam(const std::string p) {
  return p == "c" || p == "e" || p == "i" || p == "is";
}

void Tracker::handleHTTPRequest(
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

    response->addBody(fnord::StringUtil::format(
        "__cmhost='$0'; __cmuid='$1'; __cmcid='$2'; $3",
        hostname,
        rnd_.hex128(),
        rnd_.hex64(),
        ns->trackingJS()));

    return;
  }

  if (uri.path() == "/t.gif") {
    try {
      recordLogLine(ns, uri.query());
    } catch (const std::exception& e) {
      auto msg = fnord::StringUtil::format(
          "invalid tracking pixel url: $0",
          uri.query());

      fnord::log::Logger::get()->logException(fnord::log::kDebug, msg, e);
    }

    response->setStatus(fnord::http::kStatusOK);
    response->addHeader("Content-Type", "image/gif");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    response->addBody((void *) &pixel_gif, sizeof(pixel_gif));
    return;
  }

  response->setStatus(fnord::http::kStatusNotFound);
  response->addBody("not found");
}


void Tracker::addCustomer(CustomerNamespace* customer) {
  for (const auto& vhost : customer->vhosts()) {
    if (vhosts_.count(vhost) != 0) {
      RAISEF(kRuntimeError, "hostname is already registered: $0", vhost);
    }

    vhosts_[vhost] = customer;
  }
}

void Tracker::recordLogLine(
    CustomerNamespace* customer,
    const std::string& logline) {
  auto feed_key = fnord::StringUtil::format(
      "cm.tracker.log~$0",
      customer->key());

  fnord::comm::Feed* feed = nullptr;
  {
    std::unique_lock<std::mutex> l(feeds_mutex_);
    auto feed_iter = feeds_.find(feed_key);

    if (feed_iter == feeds_.end()) {
      auto f = feed_factory_->getFeed(feed_key);
      feed = f.get();
      feeds_.emplace(feed_key, std::move(f));
    } else {
      feed = feed_iter->second.get();
    }
  }

  auto pos = feed->append(logline);
  fnord::iputs("write to feed @$0 => $1", pos, logline);
}

} // namespace cm
