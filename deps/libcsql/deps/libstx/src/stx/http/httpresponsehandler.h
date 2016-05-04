/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_HTTPRESPONSEHANDLER_H
#define _libstx_HTTPRESPONSEHANDLER_H
#include "stx/thread/wakeup.h"
#include <memory>

namespace stx {
namespace http {

class HTTPResponseHandler {
public:
  virtual ~HTTPResponseHandler() {}

  virtual void onError(const std::exception& e) = 0;

  virtual void onVersion(const std::string& version) = 0;
  virtual void onStatusCode(int status_code) = 0;
  virtual void onStatusName(const std::string& status) = 0;
  virtual void onHeader(const std::string& key, const std::string& value) = 0;
  virtual void onHeadersComplete() = 0;
  virtual void onBodyChunk(const char* data, size_t size) = 0;
  virtual void onResponseComplete() = 0;

};

}
}
#endif
