/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/protobuf/MessagePrinter.h"
#include "zbase/logjoin/stages/DebugPrintStage.h"
#include "zbase/logjoin/common.h"

using namespace stx;

namespace zbase {

void DebugPrintStage::process(RefPtr<SessionContext> ctx) {
  stx::iputs("\n\n==== SESSION  $0/$1 ====", ctx->customer_key, ctx->uuid);

  for (const auto& ev : ctx->events) {
    stx::iputs(
        "    > input_event evtype=$1 time=$0 eid=$2 data=$3$4",
        ev.time,
        ev.evtype,
        ev.evid,
        ev.data.substr(0, 40),
        String(ev.data.size() > 40 ? "[...]" : ""));
  }

  for (const auto& attr : ctx->attributes()) {
    stx::iputs(
        "    > output_attr key=$0 value=$1",
        attr.first,
        attr.second);
  }

  for (const auto& ev : ctx->outputEvents()) {
    stx::iputs(
        "    > output_event evtype=$0\n$1",
        ev->obj.schema()->name(),
        msg::MessagePrinter::print(ev->obj.data(), *ev->obj.schema()));
  }
}

} // namespace zbase

