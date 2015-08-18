/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/logjoin/stages/NormalizeQueryStrings.h"
#include "zbase/logjoin/common.h"

using namespace stx;

namespace zbase {

void NormalizeQueryStrings::process(
      NormalizeFn normalize_fn,
      RefPtr<SessionContext> ctx) {
  //for (auto& q : *ctx->session.mutable_search_queries()) {
  //  if (!q.has_query_string()) {
  //    continue;
  //  }

  //  q.set_query_string_normalized(
  //      normalize_fn(
  //          (stx::Language) q.language(),
  //          q.query_string()));
  //}
}

} // namespace zbase

