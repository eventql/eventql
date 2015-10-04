/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/logjoin/stages/BuildEventsStage.h"
#include "zbase/logjoin/common.h"

using namespace stx;

namespace zbase {

void BuildEventsStage::process(RefPtr<SessionContext> ctx) {
  for (const auto& ev : ctx->events) {
    if (StringUtil::beginsWith(ev.evtype, "_")) {
      continue;
    }

    auto evptr = ctx->addOutputEvent(
        ev.time,
        SHA1::compute(ev.evid),
        ev.evtype);

    loadEvent(ev.data, evptr);
    evptr->obj.addDateTimeField("time", ev.time);
  }
}

void BuildEventsStage::loadEvent(const String& data, RefPtr<OutputEvent> ev) {
  if (StringUtil::beginsWith(data, "p")) {
    loadEventFromPlainJSON(data, ev);
    return;
  }

  RAISE(kRuntimeError, "illegal event data format");
}

void BuildEventsStage::loadEventFromPlainJSON(
    const String& data,
    RefPtr<OutputEvent> ev) {
  auto json = json::parseJSON(URI::urlDecode(data.substr(1)));
  ev->obj.fromJSON(json.begin(), json.end());
}

} // namespace zbase

