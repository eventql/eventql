/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TRACKER_H
#define _CM_TRACKER_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <fnord/base/random.h>
#include <fnord/net/http/httpservice.h>

namespace cm {
class CustomerNamespace;

class Tracker : public fnord::http::HTTPService {
public:
  static const char kUIDCookieKey[];
  static const int kUIDCookieLifetimeDays;

  Tracker();

  void handleHTTPRequest(
      fnord::http::HTTPRequest* request,
      fnord::http::HTTPResponse* response) override;

  void addCustomer(CustomerNamespace* customer);

protected:
  std::unordered_map<std::string, CustomerNamespace*> vhosts_;
  fnord::Random rnd_;
};

} // namespace cm
#endif
