/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/logjoin/stages/BuildSessionAttributes.h"
#include "zbase/logjoin/common.h"

using namespace stx;

namespace zbase {

void BuildSessionAttributes::process(RefPtr<SessionContext> ctx) {
  auto first_seen = firstSeenTime(ctx.get());
  auto last_seen = lastSeenTime(ctx.get());
  if (first_seen.isEmpty() || last_seen.isEmpty()) {
    RAISE(kRuntimeError, "session: time isn't set");
  }

  ctx->first_seen_time = first_seen.get();
  ctx->last_seen_time = last_seen.get();
  ctx->time = last_seen.get();

  for (const auto& ev : ctx->events) {
    if (ev.evtype != "__sattr") {
      continue;
    }

    URI::ParamList logline;
    URI::parseQueryString(ev.data, &logline);

    //for (const auto& p : logline) {
    //  if (p.first == "x") {
    //    experiments.emplace(p.second);
    //    continue;
    //  }
    //}

    std::string r_url;
    if (stx::URI::getParam(logline, "r_url", &r_url)) {
      ctx->setAttribute("referrer_url", r_url);
    }

    std::string r_cpn;
    if (stx::URI::getParam(logline, "r_cpn", &r_cpn)) {
      ctx->setAttribute("referrer_campaign", r_cpn);
    }

    std::string r_nm;
    if (stx::URI::getParam(logline, "r_nm", &r_nm)) {
      ctx->setAttribute("referrer_name", r_nm);
    }

    std::string cs;
    if (stx::URI::getParam(logline, "cs", &cs)) {
      ctx->setAttribute("customer_session", cs);
    }
  }
}

Option<UnixTime> BuildSessionAttributes::firstSeenTime(SessionContext* sess) {
  uint64_t t = std::numeric_limits<uint64_t>::max();

  for (const auto& e : sess->events) {
    if (e.time.unixMicros() < t) {
      t = e.time.unixMicros();
    }
  }

  if (t == std::numeric_limits<uint64_t>::max()) {
    return None<UnixTime>();
  } else {
    return Some(UnixTime(t));
  }
}

Option<UnixTime> BuildSessionAttributes::lastSeenTime(SessionContext* sess) {
  uint64_t t = std::numeric_limits<uint64_t>::min();

  for (const auto& e : sess->events) {
    if (e.time.unixMicros() > t) {
      t = e.time.unixMicros();
    }
  }

  if (t == std::numeric_limits<uint64_t>::min()) {
    return None<UnixTime>();
  } else {
    return Some(UnixTime(t));
  }
}

} // namespace zbase

