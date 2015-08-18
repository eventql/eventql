/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/stdtypes.h"
#include "logjoin/SessionContext.h"
#include "logjoin/TrackedItemVisit.h"
#include "logjoin/TrackedCartItem.h"
#include "logjoin/TrackedQuery.h"

using namespace stx;

namespace zbase {

/**
 * The max time after which a click on a query result is considered a click
 */
static const uint64_t kMaxQueryClickDelaySeconds = 180;

class SessionJoin {
public:

  static void process(RefPtr<SessionContext> session);

protected:

  static void processSearchQueryEvent(
      const TrackedEvent& event,
      Vector<TrackedQuery>* queries);

  static void processPageViewEvent(
      const TrackedEvent& event,
      Vector<TrackedItemVisit>* page_views);

  static void processCartItemsEvent(
      const TrackedEvent& event,
      Vector<TrackedCartItem>* cart_items);

};

} // namespace zbase

