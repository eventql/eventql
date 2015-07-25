/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/stdtypes.h"
#include "stx/random.h"
#include "stx/thread/FixedSizeThreadPool.h"
#include "logjoin/TrackedSession.h"
#include "logjoin/SessionContext.h"
#include "common/CustomerDirectory.h"

using namespace stx;

namespace cm {

class SessionProcessor {
public:
  typedef Function<void (RefPtr<SessionContext> ctx)> PipelineStageFn;

  SessionProcessor(
      CustomerDirectory* customer_dir);

  void addPipelineStage(PipelineStageFn fn);

  void enqueueSession(const TrackedSession& session);
  void start();
  void stop();

protected:

  void processSession(const TrackedSession& session);

  Vector<PipelineStageFn> stages_;
  CustomerDirectory* customer_dir_;
  thread::FixedSizeThreadPool tpool_;
};

} // namespace cm

