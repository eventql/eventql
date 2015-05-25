/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_BROKER_BROKERCLIENT_H
#define _FNORD_BROKER_BROKERCLIENT_H
#include <fnord-base/stdtypes.h>
#include <fnord-http/httpconnectionpool.h>
#include <fnord-feeds/FeedEntry.h>

namespace fnord {
namespace feeds {

class BrokerClient {
public:
  typedef Function<void (const FeedEntry& record)> CallbackType;

  BrokerClient(http::HTTPConnectionPool* http);

  //void insert(
  //    const List<URI>& servers,
  //    const String& feed,
  //    const Buffer& record);

  void insert(
      const URI& server,
      const String& topic,
      const Buffer& record);

  void fetch(
      const URI& server,
      const String& topic,
      size_t offset,
      size_t limit,
      CallbackType cb);

  //void fetchCursor(
  //    BrokerCursor* cursor,
  //    const String& feed,
  //    size_t offset,
  //    size_t limit,
  //    CallbackType cb);

  void monitor(
      const URI& server,
      const String& feed,
      CallbackType cb);

protected:
  http::HTTPConnectionPool* http_;
};

}
}
#endif
