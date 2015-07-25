/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "logjoin/TrackedSession.h"
#include "logjoin/JoinedSession.pb.h"
#include "common/CustomerConfig.h"

using namespace stx;

namespace cm {

struct TrackedSessionContext : public RefCounted {
  TrackedSessionContext(TrackedSession session);

  std::string uuid;

  std::string customer_key;
  RefPtr<CustomerConfigRef> customer_config;

  Vector<TrackedEvent> events;

  JoinedSession session;

};

} // namespace cm
