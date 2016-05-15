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
#include "eventql/util/rpc/RPC.h"
#include "eventql/util/json/jsonrpcrequest.h"
#include "eventql/util/json/jsonrpcresponse.h"

namespace json {

template <class ServiceType>
void JSONRPC::registerService(ServiceType* service) {
  JSONRPC::ReflectionTarget<ServiceType> target(this, service);
  reflect::MetaClass<ServiceType>::reflectMethods(&target);
}

template <class MethodType>
void JSONRPC::registerMethod(
    const std::string& method_name,
    MethodType* method_call,
    typename MethodType::ClassType* service) {
  registerMethod(method_name, [method_call, service] (
      JSONRPCRequest* req,
      JSONRPCResponse* res) {
    res->successAndReturn(method_call->call(service, *req));
  });
}

template <class ClassType>
JSONRPC::ReflectionTarget<ClassType>::ReflectionTarget(
    JSONRPC* self,
    ClassType* service) :
    self_(self),
    service_(service) {}

template <typename ClassType>
template <typename MethodType>
void JSONRPC::ReflectionTarget<ClassType>::method(MethodType* method_call) {
  self_->registerMethod(
      method_call->name(),
      method_call,
      service_);
}

template <typename ClassType>
template <typename RPCCallType>
void JSONRPC::ReflectionTarget<ClassType>::rpc(RPCCallType rpc_call) {
  auto service = service_;
  auto mname = rpc_call.method()->name();

  self_->registerMethod(mname, [rpc_call, service] (
      JSONRPCRequest* req,
      JSONRPCResponse* res) {
      RPC<
        typename RPCCallType::RPCReturnType,
        typename RPCCallType::RPCArgPackType> rpc(
        rpc_call.method()->name(),
        rpc_call.getArgs(*req));

    /*
    rpc->onReady([rpc, res] () {
      res->successAndReturn(rpc->result());
      delete rpc;
    });
    */

    rpc_call.method()->call(service, &rpc);
    rpc.wait();
    res->successAndReturn(rpc.result());
  });
}

}
