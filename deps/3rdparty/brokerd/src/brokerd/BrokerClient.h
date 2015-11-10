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
#include <stx/stdtypes.h>
#include <stx/http/httpconnectionpool.h>
#include <brokerd/Message.pb.h>
#include <brokerd/TopicCursor.pb.h>

namespace stx {
namespace feeds {

class BrokerClient {
public:
  BrokerClient(http::HTTPConnectionPool* http);

  void insert(
      const InetAddr& server,
      const String& topic,
      const Buffer& record);

  MessageList fetch(
      const InetAddr& server,
      const String& topic,
      size_t offset,
      size_t limit);

  MessageList fetchNext(
      const InetAddr& server,
      TopicCursor* cursor,
      size_t batch_size);

  void resolveCursor(TopicCursor* cursor);

  String hostID(const InetAddr& server);

  //void monitor(
  //    const InetAddr& server,
  //    const String& feed,
  //    CallbackType cb);

protected:
  http::HTTPConnectionPool* http_;
  HashMap<String, String> host_ids_;
};

}
}
#endif
