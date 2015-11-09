/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_HTTPSERVICE_H
#define _libstx_HTTPSERVICE_H
#include <stx/http/httphandler.h>
#include <stx/http/httprequest.h>
#include <stx/http/httpresponse.h>
#include <stx/http/HTTPRequestStream.h>
#include <stx/http/HTTPResponseStream.h>
#include "stx/thread/taskscheduler.h"

namespace stx {
namespace http {

class StreamingHTTPService {
public:

  virtual ~StreamingHTTPService() {}

  virtual void handleHTTPRequest(
      RefPtr<HTTPRequestStream> req,
      RefPtr<HTTPResponseStream> res) = 0;

  virtual bool isStreaming() {
    return true;
  }

};

class HTTPService : public StreamingHTTPService {
public:

  virtual ~HTTPService() {}

  virtual void handleHTTPRequest(
      HTTPRequest* req,
      HTTPResponse* res) = 0;

  void handleHTTPRequest(
      RefPtr<HTTPRequestStream> req,
      RefPtr<HTTPResponseStream> res) override;

  bool isStreaming() override {
    return false;
  }

};

class HTTPServiceHandler : public HTTPHandler {
public:
  HTTPServiceHandler(
      StreamingHTTPService* service,
      TaskScheduler* scheduler,
      HTTPServerConnection* conn,
      HTTPRequest* req);

  void handleHTTPRequest() override;

protected:
  void dispatchRequest();

  StreamingHTTPService* service_;
  TaskScheduler* scheduler_;
  HTTPServerConnection* conn_;
  HTTPRequest* req_;
};

}
}
#endif
