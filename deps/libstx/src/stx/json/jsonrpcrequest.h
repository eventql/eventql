/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_JSON_JSONRPCREQUEST_H
#define _STX_JSON_JSONRPCREQUEST_H
#include <stdlib.h>
#include <string>
#include <vector>
#include "stx/inspect.h"
#include "stx/json/json.h"

namespace stx {
namespace json {

class JSONRPCRequest {
public:
  JSONRPCRequest(JSONObject&& body);

  const std::string& id() const;
  const std::string& method() const;

  JSONObject::const_iterator paramsBegin() const;
  JSONObject::const_iterator paramsEnd() const;

  template <typename T>
  T getArg(size_t index, const std::string& name) const;

protected:
  JSONObject body_;
  JSONObject::const_iterator params_;
  std::string id_;
  std::string method_;
};

} // namespace json
} // namespace stx

#include "jsonrpcrequest_impl.h"
#endif
