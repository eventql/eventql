/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "logjoin/SessionProcessor.h"

using namespace stx;

namespace cm {

SessionProcessor::SessionProcessor(
    CustomerDirectory* customer_dir) :
    customer_dir_(customer_dir),
    tpool_(
        4,
        mkScoped(new CatchAndLogExceptionHandler("logjoind")),
        100,
        true) {}

void SessionProcessor::addPipelineStage(PipelineStageFn fn) {
  stages_.emplace_back(fn);
}

void SessionProcessor::start() {
  tpool_.start();
}

void SessionProcessor::stop() {
  tpool_.stop();
}

void SessionProcessor::enqueueSession(const TrackedSession& session) {
  // FIXPAUL: write to some form of persistent queue
  tpool_.run(std::bind(&SessionProcessor::processSession, this, session));
}

void SessionProcessor::processSession(const TrackedSession& session) {
  auto ctx = mkRef(new SessionContext(session));
  ctx->customer_config = customer_dir_->configFor(session.customer_key);

  for (const auto& stage : stages_) {
    stage(ctx);
  }
}

} // namespace cm

