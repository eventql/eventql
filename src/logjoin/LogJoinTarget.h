/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_LOGJOINTARGET_H
#define _CM_LOGJOINTARGET_H
#include "fnord-base/stdtypes.h"
#include "ItemRef.h"
#include "logjoin/TrackedSession.h"
#include "logjoin/TrackedQuery.h"


using namespace fnord;

namespace cm {

class LogJoinTarget {
public:

  void onSession(const TrackedSession& query);

  void onQuery(
      const TrackedSession& session,
      const TrackedQuery& query);

  void onItemVisit(
      const TrackedSession& session,
      const TrackedItemVisit& item_visit);

  void onItemVisit(
      const TrackedSession& session,
      const TrackedItemVisit& item_visit,
      const TrackedQuery& query);
};
} // namespace cm

#endif
