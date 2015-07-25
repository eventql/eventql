/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "logjoin/SessionPipeline.h"

using namespace stx;

namespace cm {

void SessionPipeline::addStage(PipelineStageFn fn) {
  stages_.emplace_back(fn);
}

RefPtr<TrackedSessionContext> SessionPipeline::processSession(
    TrackedSession session) {
  auto ctx = mkRef(new TrackedSessionContext);
  ctx->tracked_session = session;

  for (const auto& stage : stages_) {
    stage(ctx);
  }

  return ctx;
}

} // namespace cm

