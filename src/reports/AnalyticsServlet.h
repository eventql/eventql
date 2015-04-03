/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CTRBYPOSITIONSERVLET_H
#define _CM_CTRBYPOSITIONSERVLET_H
#include "fnord-base/VFS.h"
#include "fnord-http/httpservice.h"
#include "fnord-json/json.h"

using namespace fnord;

namespace cm {

/**
 *  GET /reports/ctr_by_position
 *
 *    params: customer, language, start_time, end_time, device_type..., test_group...
 *
 */
class AnalyticsServlet : public fnord::http::HTTPService {
public:

  AnalyticsServlet(VFS* vfs);

  void handleHTTPRequest(
      fnord::http::HTTPRequest* req,
      fnord::http::HTTPResponse* res);

protected:
  VFS* vfs_;
};

}
#endif
