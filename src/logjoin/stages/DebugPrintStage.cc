/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "logjoin/stages/DebugPrintStage.h"
#include "logjoin/common.h"

using namespace stx;

namespace cm {

void DebugPrintStage::process(RefPtr<TrackedSessionContext> ctx) {
  stx::iputs("\n\n==== SESSION ====", 1);
  ctx->tracked_session.debugPrint();
  stx::iputs("$0", ctx->joined_session.DebugString());
}

} // namespace cm

