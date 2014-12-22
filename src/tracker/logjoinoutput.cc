/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord/base/inspect.h>
#include "logjoinoutput.h"

namespace cm {

void LogJoinOutput::recordJoinedQuery(
    CustomerNamespace* customer,
    const std::string& uid,
    const std::string& eid,
    const TrackedQuery& query) {
  fnord::iputs("joined query...", 1);
}

void LogJoinOutput::recordJoinedItemVisit(
    CustomerNamespace* customer,
    const std::string& uid,
    const std::string& eid,
    const TrackedItemVisit& visit) {
  fnord::iputs("joined item visit...", 1);
}

void LogJoinOutput::recordJoinedSession(
    CustomerNamespace* customer,
    const std::string& uid,
    const TrackedSession& session) {
  fnord::iputs("joined session...", 1);
}

} // namespace cm
