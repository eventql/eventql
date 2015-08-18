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
#include "stx/rpc/RPCStub.h"

namespace stx {
namespace rpc {

class RPCService : public RPCStub {
public:

  RefPtr<RPCRequest> getRPC(
      const String& method,
      const Serializable& params) override;

  template <class ParamType>
  void registerMethod(
      const String& job_name,
      Function<void (const ParamType& params, RPCContext* ctx)> fn);

protected:

  struct RPCFactoryMethods {
    Function<RefPtr<RPCRequest> (const Serializable& params)> ctor_ref;
    Function<RefPtr<RPCRequest> (InputStream* is)> ctor_io;
  };

  mutable std::mutex mutex_;
  HashMap<String, RPCFactoryMethods> methods_;
};

} // namespace rpc
} // namespace stx

#include "RPCService_impl.h"
