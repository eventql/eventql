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
#include "stx/SHA1.h"
#include "zbase/logjoin/TrackedSession.h"
#include "zbase/CustomerConfig.h"

using namespace stx;

namespace zbase {

struct OutputEvent : public RefCounted {
  OutputEvent(
      UnixTime _time,
      SHA1Hash _evid,
      RefPtr<msg::MessageSchema> schema);

  UnixTime time;
  SHA1Hash evid;
  msg::DynamicMessage obj;
};

struct SessionContext : public RefCounted {
  SessionContext(
      TrackedSession session,
      RefPtr<CustomerConfigRef> cconf);

  std::string uuid;

  std::string customer_key;
  RefPtr<CustomerConfigRef> customer_config;

  UnixTime time;
  UnixTime first_seen_time;
  UnixTime last_seen_time;

  Vector<TrackedEvent> events;

  const Vector<RefPtr<OutputEvent>>& outputEvents() const;
  RefPtr<OutputEvent> addOutputEvent(
      UnixTime time,
      SHA1Hash evid,
      const String& evtype);

  const HashMap<String, String>& attributes() const;
  void setAttribute(const String& key, const String& value);

protected:
  Vector<RefPtr<OutputEvent>> output_events_;
  HashMap<String, String> attributes_;
};

} // namespace zbase
