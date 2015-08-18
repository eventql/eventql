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
#include "stx/thread/DelayedQueue.h"
#include "logjoin/TrackedSession.h"
#include "logjoin/SessionContext.h"
#include "zbase/ConfigDirectory.h"

using namespace stx;

namespace cm {

class SessionProcessor {
public:
  typedef Function<void (RefPtr<SessionContext> ctx)> PipelineStageFn;

  SessionProcessor(
      ConfigDirectory* customer_dir,
      const String& spool_path);

  void addPipelineStage(PipelineStageFn fn);

  void enqueueSession(const TrackedSession& session);
  void start();
  void stop();

protected:

  void work();

  void processSession(const SHA1Hash& skey);
  void processSession(const TrackedSession& session);

  ConfigDirectory* customer_dir_;
  String spool_path_;
  Vector<PipelineStageFn> stages_;
  thread::DelayedQueue<SHA1Hash> queue_;
  Vector<std::thread> threads_;
  std::atomic<bool> running_;
};

} // namespace cm

