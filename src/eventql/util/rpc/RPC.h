/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#ifndef _STX_COMM_RPC_H
#define _STX_COMM_RPC_H
#include <functional>
#include <memory>
#include <mutex>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "eventql/util/status.h"
#include "eventql/util/stdtypes.h"
#include "eventql/util/thread/future.h"
#include "eventql/util/thread/wakeup.h"

class RPCChannel;

class AnyRPC : public RefCounted {
public:
  virtual ~AnyRPC();
  AnyRPC(const std::string& method);

  void wait();
  void onReady(Function<void()> callback);
  void raiseIfError() const;

  const std::string& method() const;

  void ready(const Buffer& result) noexcept;
  void error(const std::exception& e);
  void error(const Status& status);

  bool isSuccess() const;
  bool isFailure() const;
  const Status& status() const;

  const Buffer& encoded();

protected:
  void ready() noexcept;

  Status status_;
  String method_;
  bool is_ready_;
  bool autodelete_;
  std::mutex mutex_;
  Wakeup ready_wakeup_;
  Buffer encoded_request_;
  Function<void(const Buffer&)> decode_fn_;
};

template <typename _ResultType, typename _ArgPackType>
class RPC : public AnyRPC {
public:
  typedef _ResultType ResultType;
  typedef _ArgPackType ArgPackType;
  typedef Function<ScopedPtr<ResultType> (const Buffer&)> DecodeFnType;

  RPC(const std::string& method, const ArgPackType& arguments);
  //RPC(const std::string& method, Buffer&& encoded, DecodeFnType decoder);
  RPC(const RPC<ResultType, ArgPackType>& other);

  void onSuccess(Function<void(const RPC<ResultType, ArgPackType>& rpc)> fn);
  void onError(Function<void(const Status& status)> fn);

  template <typename Codec>
  void encode();

  void success(ScopedPtr<ResultType> result) noexcept;

  const ResultType& result() const;
  const ArgPackType& args() const;

protected:
  ArgPackType args_;
  ScopedPtr<ResultType> result_;
};

template <class Encoder, class ReturnType, typename... ArgTypes>
AutoRef<RPC<ReturnType, std::tuple<ArgTypes...>>> mkRPC(
    const std::string& method,
    ArgTypes... args);

template <class Encoder, class MethodCall>
AutoRef<
    RPC<
        typename MethodCall::ReturnType,
        typename MethodCall::ArgPackType>> mkRPC(
    const MethodCall* method,
    typename MethodCall::ArgPackType args);

template <
    class Encoder,
    typename ClassType,
    typename ReturnType,
    typename... ArgTypes>
AutoRef<RPC<ReturnType, std::tuple<ArgTypes...>>> mkRPC(
  ReturnType (ClassType::* method)(ArgTypes...),
  ArgTypes... args);

#include "RPC_impl.h"
#endif
