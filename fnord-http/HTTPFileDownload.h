/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_HTTP_FILEDOWNLOAD_H
#define _FNORD_HTTP_FILEDOWNLOAD_H
#include <fnord-base/uri.h>
#include <fnord-http/httpmessage.h>
#include "fnord-http/httprequest.h"
#include "fnord-http/httpresponse.h"
#include "fnord-http/httpstats.h"
#include "fnord-http/httpconnectionpool.h"
#include <string>

namespace fnord {
namespace http {

class HTTPFileDownload {
public:

  HTTPFileDownload(
      const HTTPRequest& http_req,
      const String& output_file_);

  Future<HTTPResponse> download(HTTPConnectionPool* http);

protected:
  HTTPRequest http_req_;
  String output_file_;
};

}
}
#endif
