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
#ifndef _STX_JSON_JSONRPC_H
#define _STX_JSON_JSONRPC_H
#include <functional>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "eventql/util/json/jsonrpchttpadapter.h"
#include "eventql/util/reflect/reflect.h"

namespace stx {
namespace json {
class JSONRPCRequest;
class JSONRPCResponse;

class JSONRPC {
public:

  JSONRPC();

  void dispatch(JSONRPCRequest* req, JSONRPCResponse* res);

  template <class ServiceType>
  void registerService(ServiceType* service);

  template <class MethodType>
  void registerMethod(
      const std::string& method_name,
      MethodType* method_call,
      typename MethodType::ClassType* service);

  void registerMethod(
      const std::string& method,
      std::function<void (JSONRPCRequest* req, JSONRPCResponse* res)> handler);

protected:
  template <class ClassType>
  class ReflectionTarget {
  public:
    ReflectionTarget(JSONRPC* self, ClassType* service);

    template <typename MethodType>
    void method(MethodType* method_call);

    template <typename RPCCallType>
    void rpc(RPCCallType rpccall);

  protected:
    JSONRPC* self_;
    ClassType* service_;
  };

  std::unordered_map<
      std::string,
      std::function<void (JSONRPCRequest* req, JSONRPCResponse* res)>>
      handlers_;
};

} // namespace json
} // namespace stx

#include "jsonrpc_impl.h"
#endif
