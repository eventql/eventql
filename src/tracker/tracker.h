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
#include <fnord/net/http/httphandler.h>

namespace cm {

class Tracker : public fnord::http::HTTPHandler {
public:

  Tracker();

  bool handleHTTPRequest(
      fnord::http::HTTPRequest* request,
      fnord::http::HTTPResponse* response) override;

};

} // namespace cm
#endif
