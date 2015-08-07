/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "stx/stdtypes.h"
#include "stx/autoref.h"
#include "stx/duration.h"
#include "stx/exception.h"
#include "stx/Serializable.h"
#include "stx/io/inputstream.h"

namespace stx {
namespace rpc {
class RPCContext;

class RPCRequest : public RefCounted {
  friend class RPCContext;
public:

  RPCRequest(Function<void (RPCContext* ctx)> call_fn);

  void run();
  void cancel();

  void wait() const;
  bool waitFor(const Duration& timeout) const;

  void onReady(Function<void ()> on_ready);

  template <typename EventType>
  void onEvent(
      const String& event_name,
      const EventType& event);

protected:

  void returnSuccess();
  void returnError(const StandardException& e);
  void returnError(const ExceptionType type, const String& message);

  Function<void (RPCContext* ctx)> call_fn_;
  bool cancelled_;
  bool ready_;
  const char* error_;
  String error_message_;
  bool running_;
  HashMap<String, Function<void (const Serializable& result)>> on_event_;
  Function<void ()> on_ready_;
  Function<void ()> on_cancel_;
  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
};


} // namespace rpc
} // namespace stx

