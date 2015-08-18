/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_LOOKUPSERVLET_H
#define _CM_LOOKUPSERVLET_H
#include "stx/http/httpservice.h"
#include "stx/json/json.h"
#include "stx/mdb/MDB.h"

using namespace stx;

namespace zbase {

class LookupServlet : public http::HTTPService {
public:

  LookupServlet(const String& cmdata);

  void handleHTTPRequest(
      http::HTTPRequest* req,
      http::HTTPResponse* res);

protected:

  void lookupSellerStats(
      http::HTTPRequest* req,
      http::HTTPResponse* res);

  String cmdata_;
};

}
#endif
