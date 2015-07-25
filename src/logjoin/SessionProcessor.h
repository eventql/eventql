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
#include "stx/Currency.h"
#include "stx/Language.h"
#include "stx/random.h"
#include "stx/mdb/MDB.h"
#include "stx/protobuf/MessageSchema.h"
#include <inventory/ItemRef.h>
#include "logjoin/TrackedSession.h"
#include "logjoin/TrackedQuery.h"
#include "logjoin/SessionPipeline.h"
#include "inventory/DocStore.h"
#include "inventory/IndexChangeRequest.h"
#include "inventory/DocIndex.h"
#include <inventory/ItemRef.h>
#include "stx/thread/FixedSizeThreadPool.h"

using namespace stx;

namespace cm {

class SessionProcessor {
public:
  typedef Function<void (RefPtr<TrackedSessionContext> ctx)> PipelineStageFn;

  SessionProcessor(RefPtr<SessionPipeline> pipeline);

  void enqueueSession(const TrackedSession& session);
  void start();
  void stop();

protected:

  void processSession(const TrackedSession& session);

  RefPtr<SessionPipeline> pipeline_;
  thread::FixedSizeThreadPool tpool_;
};
} // namespace cm

