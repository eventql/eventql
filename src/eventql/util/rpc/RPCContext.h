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
#include "eventql/util/stdtypes.h"
#include "eventql/util/autoref.h"
#include "eventql/util/duration.h"
#include "eventql/util/exception.h"
#include "eventql/util/Serializable.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/util/rpc/RPCRequest.h"

namespace stx {
namespace rpc {

class RPCContext {
public:

  RPCContext(RPCRequest* rpc);

  bool isCancelled() const;
  void onCancel(Function<void ()> fn);

  template <typename EventType>
  void sendEvent(const String& event_name, const EventType& data);

protected:
  RPCRequest* rpc_;
};

} // namespace rpc
} // namespace stx

