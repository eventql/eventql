/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_HTTPCONNECTIONHANDLER_H
#define _libstx_HTTPCONNECTIONHANDLER_H
#include <stx/http/httprequest.h>
#include <stx/http/httpresponse.h>

namespace stx {
namespace http {

class HTTPServerConnectionHandler {
public:

  virtual ~HTTPServerConnectionHandler() {}

  virtual bool handleHTTPRequest(
      HTTPRequest* request,
      HTTPResponse* response) = 0;

};

}
}
#endif
