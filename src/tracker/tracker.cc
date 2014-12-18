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
#include "fnord/base/random.h"
#include "customernamespace.h"

namespace cm {

const char Tracker::kUIDCookieKey[] = "_u";
const int Tracker::kUIDCookieLifetimeDays = 365 * 5;

Tracker::Tracker() {}

const unsigned char pixel_gif[42] = {
  0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x21, 0xf9, 0x04, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
  0x00, 0x02, 0x01, 0x44, 0x00, 0x3b
};

bool Tracker::handleHTTPRequest(
    fnord::http::HTTPRequest* request,
    fnord::http::HTTPResponse* response) {
  /* find namespace */
  CustomerNamespace* ns = nullptr;
  const auto hostname = request->getHeader("host");
  auto ns_iter = vhosts_.find(hostname);
  if (ns_iter == vhosts_.end()) {
    return false;
  } else {
    ns = ns_iter->second;
  }

  fnord::URI uri(request->uri());
  fnord::iputs("uid: namespace $0, req $1", ns, request->uri());

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

    return true;
  }

  if (uri.path() == "/t.gif") {
    response->setStatus(fnord::http::kStatusOK);
    response->addHeader("Content-Type", "image/gif");
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    response->addBody((void *) &pixel_gif, sizeof(pixel_gif));
    return true;
  }

  response->setStatus(fnord::http::kStatusNotFound);
  return true;
}

void Tracker::addCustomer(CustomerNamespace* customer) {
  for (const auto& vhost : customer->vhosts()) {
    if (vhosts_.count(vhost) != 0) {
      RAISEF(kRuntimeError, "hostname is already registered: $0", vhost);
    }

    vhosts_[vhost] = customer;
  }
}

} // namespace cm
