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
      ">> session uid=$0 last_access=$1",
      uid,
      fnord::DateTime(last_access_unix_micros));

  fnord::iputs(" > queries: ", 1);
  for (const auto& pair : queries) {
    const auto& query = pair.second;
    fnord::iputs(
        "    > query time=$0 flushed=$2\n        > attrs: $1",
        query.time,
        query.attrs,
        query.flushed);

    for (const auto& item : query.items) {
      fnord::iputs(
          "        > qitem: id=$0 clicked=$1 position=$2",
          item.item,
          item.clicked,
          item.position);
    }
  }

/*
struct TrackedQueryItem {
  ItemRef item;
  bool clicked;
  int position;
  int variant;
};

struct TrackedQuery {
  fnord::DateTime time;
  std::vector<TrackedQueryItem> items;
  std::vector<std::string> attrs;
  bool flushed;
};
*/
  fnord::iputs(" > item views: ", 1);

  fnord::iputs("", 1);
}

} // namespace cm
