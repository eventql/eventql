/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_DEMOQUERYSERVLET_H
#define _CM_DEMOQUERYSERVLET_H
#include <unistd.h>
#include "fnord-http/httpservice.h"
#include "fnord-http/HTTPSSEStream.h"

using namespace fnord;
namespace cm {

class demoQueryServlet : public http::StreamingHTTPService {
public:

  void handleHTTPRequest(
      RefPtr<http::HTTPRequestStream> req_stream,
      RefPtr<http::HTTPResponseStream> res_stream);

};

}
#endif

