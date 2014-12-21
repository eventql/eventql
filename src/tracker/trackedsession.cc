/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "trackedsession.h"
#include <fnord/base/inspect.h>

namespace cm {

void TrackedSession::debugPrint(const std::string& uid) {
  fnord::iputs(
      ">> session uid=$0 last_seen=$1",
      uid,
      fnord::DateTime(last_seen_unix_micros));

  fnord::iputs(" > queries: ", 1);
  for (const auto& pair : queries) {
    const auto& query = pair.second;
    fnord::iputs(
        "    > query time=$0 flushed=$2 eid=$3\n        > attrs: $1",
        query.time,
        query.attrs,
        query.flushed,
        pair.first);

    for (const auto& item : query.items) {
      fnord::iputs(
          "        > qitem: id=$0 clicked=$1 position=$2 variant=$3",
          item.item,
          item.clicked,
          item.position,
          item.variant);
    }
  }

  fnord::iputs(" > item visits: ", 1);
  for (const auto& pair : item_visits) {
    const auto& view = pair.second;

    fnord::iputs(
        "    > visit: item=$0 time=$1 eid=$3 attrs=$2",
        view.item,
        view.time,
        view.attrs,
        pair.first);
  }

  fnord::iputs("", 1);
}

} // namespace cm
