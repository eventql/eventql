/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/http/httpservice.h>
#include <stx/http/httpstats.h>
#include <stx/SHA1.h>
#include <zbase/core/PartitionMap.h>

using namespace stx;
namespace zbase {

class StatusServlet : public stx::http::HTTPService {
public:

  StatusServlet(
      ServerConfig* config,
      PartitionMap* pmap,
      http::HTTPServerStats* http_stats);

  void handleHTTPRequest(
      http::HTTPRequest* request,
      http::HTTPResponse* response) override;

protected:

  void renderDashboard(
      http::HTTPRequest* request,
      http::HTTPResponse* response);

  void renderNamespacesPage(
      http::HTTPRequest* request,
      http::HTTPResponse* response);

  void renderNamespacePage(
      const String& db_namespace,
      http::HTTPRequest* request,
      http::HTTPResponse* response);

  void renderTablePage(
      const String& db_namespace,
      const String& table_name,
      http::HTTPRequest* request,
      http::HTTPResponse* response);

  void renderPartitionPage(
      const String& db_namespace,
      const String& table_name,
      const SHA1Hash& partition,
      http::HTTPRequest* request,
      http::HTTPResponse* response);

  ServerConfig* config_;
  PartitionMap* pmap_;
  http::HTTPServerStats* http_stats_;
};

}
