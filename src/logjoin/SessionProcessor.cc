/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "logjoin/SessionProcessor.h"

using namespace stx;

namespace cm {

SessionProcessor::SessionProcessor(
    CustomerDirectory* customer_dir) :
    customer_dir_(customer_dir),
    queue_(100),
    running_(false) {}
    //tpool_(
    //    4,
    //    mkScoped(new CatchAndLogExceptionHandler("logjoind")),
    //    100,
    //    true) {}

void SessionProcessor::addPipelineStage(PipelineStageFn fn) {
  stages_.emplace_back(fn);
}

void SessionProcessor::start() {
  running_ = true;

  for (int i = 0; i < 4; ++i) {
    threads_.emplace_back(std::bind(&SessionProcessor::work, this));
  }
}

void SessionProcessor::stop() {
  running_ = false;
  queue_.wakeup();

  for (auto& t : threads_) {
    t.join();
  }
}

void SessionProcessor::work() {
  while (running_) {
    auto skey = queue_.interruptiblePop();
    if (skey.isEmpty()) {
      continue;
    }

    try {
      processSession(skey.get());
    } catch (const std::exception& e) {
      logError("logjoin", e, "error while processing session");
      queue_.insert(
          skey.get(),
          WallClock::unixMicros() + 30 * kMicrosPerSecond,
          false);
    }
  }
}

void SessionProcessor::enqueueSession(const TrackedSession& session) {
  auto skey = SHA1::compute(session.customer_key + "~" + session.uuid);

  logDebug(
      "logjoin",
      "Enqueueing session for processing: $0/$1",
      session.customer_key,
      session.uuid);

  queue_.insert(skey, WallClock::now(), true);
}

void SessionProcessor::processSession(const SHA1Hash& skey) {
  logDebug(
      "logjoin",
      "Processing session: $1/$2 ($0)",
      skey.toString());
}

void SessionProcessor::processSession(const TrackedSession& session) {
  auto ctx = mkRef(
      new SessionContext(
          session,
          customer_dir_->configFor(session.customer_key)));

  for (const auto& stage : stages_) {
    stage(ctx);
  }
}

} // namespace cm

