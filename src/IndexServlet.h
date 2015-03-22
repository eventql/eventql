/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_INDEXSERVLET_H
#define _CM_INDEXSERVLET_H
#include "fnord-base/autoref.h"
#include "fnord-http/httpservice.h"
#include "fnord-fts/Analyzer.h"
#include "IndexReader.h"

using namespace fnord;

namespace cm {

class IndexServlet : public fnord::http::HTTPService {
public:

  IndexServlet(
      RefPtr<cm::IndexReader> index,
      RefPtr<fts::Analyzer> r);

  void handleHTTPRequest(
      fnord::http::HTTPRequest* req,
      fnord::http::HTTPResponse* res) override;

protected:

  void searchQuery(
      http::HTTPRequest* request,
      http::HTTPResponse* response,
      URI* uri);

  RefPtr<cm::IndexReader> index_;
  RefPtr<fts::Analyzer> analyzer_;
};

}
#endif
