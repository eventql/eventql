/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "eventql/util/http/httpservice.h"
#include "eventql/util/json/json.h"
#include "eventql/util/random.h"
#include "eventql/AnalyticsAuth.h"
#include "eventql/ConfigDirectory.h"
#include "eventql/docdb/DocumentDB.h"

using namespace stx;

namespace zbase {

class DocumentDBServlet {
public:

  DocumentDBServlet(DocumentDB* docdb);

  void handle(
      const AnalyticsSession& session,
      const stx::http::HTTPRequest* req,
      stx::http::HTTPResponse* res);

protected:

  void fetchDocument(
      const SHA1Hash& uuid,
      const URI& uri,
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void createDocument(
      const URI& uri,
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void updateDocument(
      const SHA1Hash& uuid,
      const URI& uri,
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void listDocuments(
      const AnalyticsSession& session,
      const http::HTTPRequest* req,
      http::HTTPResponse* res);

  void renderDocument(
      const AnalyticsSession& session,
      const Document& doc,
      json::JSONOutputStream* json,
      bool return_content = true);

  DocumentDB* docdb_;
};

}
