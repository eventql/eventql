/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/protobuf/DynamicMessage.h"
#include "logjoin/TrackedSession.h"
#include "logjoin/JoinedSession.pb.h"
#include "common/CustomerConfig.h"

using namespace stx;

namespace cm {

struct SessionContext : public RefCounted {
  SessionContext(
      TrackedSession session,
      RefPtr<CustomerConfigRef> cconf);

  std::string uuid;

  std::string customer_key;
  RefPtr<CustomerConfigRef> customer_config;

  Vector<TrackedEvent> events;
  
  JoinedSession session;

  const Vector<ScopedPtr<msg::DynamicMessage>>& outputEvents() const;
  msg::DynamicMessage* addOutputEvent(const String& evtype);

  const HashMap<String, String>& attributes() const;
  void setAttribute(const String& key, const String& value);

protected:
  Vector<ScopedPtr<msg::DynamicMessage>> output_events_;
  HashMap<String, String> attributes_;
};

} // namespace cm
