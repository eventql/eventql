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
#include "logjoin/TrackedSession.h"
#include "logjoin/TrackedQuery.h"

using namespace stx;

namespace cm {

class SessionJoin {
public:

  static void process(RefPtr<TrackedSessionContext> session);

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

} // namespace cm

