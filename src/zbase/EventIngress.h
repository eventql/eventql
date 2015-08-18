/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <zbase/core/TSDBClient.h>
#include <zbase/ECommerceTransaction.pb.h>

using namespace stx;

namespace zbase {

class EventIngress {
public:

  struct Event {
    Event(
        const SHA1Hash& _id,
        const UnixTime& _time,
        const Buffer& _data) :
        id(_id),
        time(_time),
        data(_data) {}

    const SHA1Hash id;
    const UnixTime time;
    const Buffer data;
  };

  EventIngress(zbase::TSDBClient* tsdb);

  void insertEvents(
      const String& customer,
      const String& event_type,
      const Buffer& data);

  void insertECommerceTransactionsJSON(
      const String& customer,
      const Buffer& data);

protected:

  void insertEvents(
      const String& customer,
      const String& table_name,
      const List<Event>& events);

  zbase::TSDBClient* tsdb_;
};


} // namespace zbase
