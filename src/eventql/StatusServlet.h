/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/http/httpservice.h>
#include <eventql/util/http/httpstats.h>
#include <eventql/util/SHA1.h>
#include <eventql/core/PartitionMap.h>

#include "eventql/eventql.h"
namespace eventql {

class StatusServlet : public http::HTTPService {
public:

  StatusServlet(
      ServerConfig* config,
      PartitionMap* pmap,
      http::HTTPServerStats* http_server_stats,
      http::HTTPClientStats* http_client_stats);

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
  http::HTTPServerStats* http_server_stats_;
  http::HTTPClientStats* http_client_stats_;
};

}
