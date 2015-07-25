/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/protobuf/MessageBuilder.h"
#include "stx/protobuf/MessageObject.h"
#include "stx/protobuf/MessageEncoder.h"
#include "stx/protobuf/MessageDecoder.h"
#include "stx/protobuf/MessagePrinter.h"
#include "stx/protobuf/msg.h"
#include "logjoin/SessionProcessor.h"
#include "common.h"

using namespace stx;

namespace cm {

SessionProcessor::SessionProcessor(
    RefPtr<SessionPipeline> pipeline) :
    pipeline_(pipeline),
    tpool_(
        4,
        mkScoped(new CatchAndLogExceptionHandler("logjoind")),
        100,
        true) {}

void SessionProcessor::start() {
  tpool_.start();
}

void SessionProcessor::stop() {
  tpool_.stop();
}

void SessionProcessor::enqueueSession(const TrackedSession& session) {
  // FIXPAUL: write to some form of persistent queue
  tpool_.run([this, session] {
    pipeline_->processSession(session);
  });
}

} // namespace cm

