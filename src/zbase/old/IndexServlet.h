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
#include "stx/autoref.h"
#include "stx/http/httpservice.h"
#include "zbase/util/Analyzer.h"
#include "IndexReader.h"

using namespace stx;

namespace zbase {

class IndexServlet : public stx::http::HTTPService {
public:

  IndexServlet(
      RefPtr<zbase::IndexReader> index,
      RefPtr<fts::Analyzer> r);

  void handleHTTPRequest(
      stx::http::HTTPRequest* req,
      stx::http::HTTPResponse* res) override;

protected:

  void searchQuery(
      http::HTTPRequest* request,
      http::HTTPResponse* response,
      URI* uri);

  void fetchDoc(
      http::HTTPRequest* request,
      http::HTTPResponse* response,
      URI* uri);

  void fetchDocs(
      http::HTTPRequest* request,
      http::HTTPResponse* response,
      URI* uri);

  RefPtr<zbase::IndexReader> index_;
  RefPtr<fts::Analyzer> analyzer_;
};

}
#endif
