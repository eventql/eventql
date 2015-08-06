/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "stx/stdtypes.h"
#include "stx/rpc/distq/QueryContext.h"

using namespace stx;

namespace stx {
namespace distq {

class QueryExecutor {
public:

  /**
   * Execute on the local (calling) thread (blocks until finished)
   */
  template <typename ParamType, typename ResultType>
  void executeLocal(RefPtr<TypedQueryContext<ParamType, ResultType>> ctx);

  /**
   * Execute on the local thread pool (returns immediately)
   */
  template <typename ParamType, typename ResultType>
  void executeLocal(RefPtr<TypedQueryContext<ParamType, ResultType>> ctx);

  /**
   * Execute on some remote machine (returns immediately)
   */
  template <typename ParamType, typename ResultType>
  void executeRemote(
      RefPtr<TypedQueryContext<ParamType, ResultType>> ctx,
      const URI& url);

  /**
   * Register query/method
   */
  template <typename ParamType, typename ResultType>
  void registerMethod(
      const String& method,
      Function<void (RefPtr<TypedQueryContext<ParamType, ResultType>> fn);

};

} // namespace distq
} // namespace stx

