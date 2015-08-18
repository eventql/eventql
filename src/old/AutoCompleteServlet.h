/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_AUTOCOMPLETESERVLET_H
#define _CM_AUTOCOMPLETESERVLET_H
#include "stx/http/httpservice.h"
#include "stx/json/json.h"
#include "ModelCache.h"
#include "AutoCompleteModel.h"

using namespace stx;

namespace zbase {

/**
 *  GET /autocomplete
 *
 */
class AutoCompleteServlet : public stx::http::HTTPService {
public:

  AutoCompleteServlet(ModelCache* models);

  void handleHTTPRequest(
      stx::http::HTTPRequest* req,
      stx::http::HTTPResponse* res);

protected:

  void generateURL(AutoCompleteResult* result);

  ModelCache* models_;
  stats::Counter<uint64_t> stat_requests_total_;
};

}
#endif
