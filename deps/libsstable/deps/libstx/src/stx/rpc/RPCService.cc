
/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/rpc/RPCService.h"

namespace stx {
namespace rpc {

RefPtr<RPCRequest> RPCService::getRPC(
    const String& method,
    const Serializable& params) {
  std::unique_lock<std::mutex> lk(mutex_);

  auto iter = methods_.find(method);
  if (iter == methods_.end()) {
    RAISEF(kNotFoundError, "job not found: $0", method);
  }

  return iter->second.ctor_ref(params);
}

} // namespace rpc
} // namespace stx

