/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/protobuf/MessageObject.h"
#include "stx/protobuf/MessageSchema.h"
#include "logjoin/TrackedSession.h"
#include "logjoin/JoinedSession.pb.h"
#include "common/CustomerConfig.h"

using namespace stx;

namespace cm {

struct JoinedEvent {
  JoinedEvent(RefPtr<msg::MessageSchema> _schema);

  void addUInt32Field(const String& name, uint32_t val);

  RefPtr<msg::MessageSchema> schema;
  msg::MessageObject data;
};

struct SessionContext : public RefCounted {
  SessionContext(TrackedSession session);

  std::string uuid;

  std::string customer_key;
  RefPtr<CustomerConfigRef> customer_config;

  Vector<TrackedEvent> events;

  HashMap<String, String> output_attrs;

  JoinedSession session;

  JoinedEvent* addOutputEvent(const String& evtype);

protected:

  Vector<ScopedPtr<JoinedEvent>> output_events_;
};

} // namespace cm
