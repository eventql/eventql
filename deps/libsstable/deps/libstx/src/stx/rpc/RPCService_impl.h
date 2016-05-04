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

namespace stx {
namespace rpc {

template <class ParamType>
void RPCService::registerMethod(
    const String& job_name,
    Function<void (const ParamType& params, RPCContext* ctx)> fn) {
  RPCFactoryMethods fmethods;

  fmethods.ctor_ref = [fn] (const Serializable& any_params) -> RefPtr<RPCRequest> {
    const auto& params = dynamic_cast<const ParamType&>(any_params);
    return new RPCRequest([fn, params] (RPCContext* ctx) {
      fn(params, ctx);
    });
  };

  std::unique_lock<std::mutex> lk(mutex_);
  methods_.emplace(job_name, fmethods);
}

} // namespace rpc
} // namespace stx

