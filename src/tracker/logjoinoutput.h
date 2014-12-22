/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_LOGJOINOUTPUT_H
#define _CM_LOGJOINOUTPUT_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include "itemref.h"
#include "trackedsession.h"
#include "trackedquery.h"

namespace cm {
class CustomerNamespace;

class LogJoinOutput {
public:
  LogJoinOutput() {}

  void recordJoinedQuery(
      CustomerNamespace* customer,
      const std::string& uid,
      const std::string& eid,
      const TrackedQuery& query);

  void recordJoinedItemVisit(
      CustomerNamespace* customer,
      const std::string& uid,
      const std::string& eid,
      const TrackedItemVisit& visit);

  void recordJoinedSession(
      CustomerNamespace* customer,
      const std::string& uid,
      const TrackedSession& session);

};

} // namespace cm
#endif
