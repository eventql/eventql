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

namespace zbase {

void DebugPrintStage::process(RefPtr<SessionContext> ctx) {
  stx::iputs("\n\n==== SESSION  $0/$1 ====", ctx->customer_key, ctx->uuid);

  for (const auto& ev : ctx->events) {
    stx::iputs(
        "    > event time=$0 evtype=$1 eid=$2 data=$3$4",
        ev.time,
        ev.evtype,
        ev.evid,
        ev.data.substr(0, 40),
        String(ev.data.size() > 40 ? "[...]" : ""));
  }

  stx::iputs("$0", ctx->session.DebugString());
}

} // namespace zbase

