/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
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
#include "stx/status.h"
#include "stx/stdtypes.h"
#include "stx/thread/future.h"
#include "stx/thread/wakeup.h"

namespace stx {
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

} // namespace stx

#include "RPC_impl.h"
#endif
