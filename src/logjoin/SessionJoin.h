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

  //void insertLogline(
  //    const UnixTime& time,
  //    const String& evtype,
  //    const String& evid,
  //    const URI::ParamList& logline);

  //void insertQuery(const TrackedQuery& query);
  //void insertItemVisit(const TrackedItemVisit& visit);
  //void insertCartVisit(const Vector<TrackedCartItem>& new_cart_items);
};

} // namespace cm

