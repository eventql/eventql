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
#include "fnord-http/httpservice.h"
#include "fnord-json/json.h"
#include "analytics/TermInfo.h"
#include "ModelCache.h"
#include "AutoCompleteModel.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include <fnord-fts/Analyzer.h>

using namespace fnord;

namespace cm {

/**
 *  GET /autocomplete
 *
 */
class AutoCompleteServlet : public fnord::http::HTTPService {
public:

  AutoCompleteServlet(
      ModelCache* models,
      RefPtr<fts::Analyzer> analyzer);

  void handleHTTPRequest(
      fnord::http::HTTPRequest* req,
      fnord::http::HTTPResponse* res);

protected:

  void generateURL(AutoCompleteResult* result);

  ModelCache* models_;
  RefPtr<fts::Analyzer> analyzer_;
};

}
#endif
