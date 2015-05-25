/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-feeds/BrokerClient.h>
#include <fnord-msg/msg.h>

namespace fnord {
namespace feeds {

BrokerClient::BrokerClient(http::HTTPConnectionPool* http) : http_(http) {}

//void BrokerClient::insert(
//    const List<URI>& servers, // FIXPAUL serverlist class
//    const String& feed,
//    const Buffer& record) {
//
//}

void BrokerClient::insert(
    const URI& server,
    const String& topic,
    const Buffer& record) {
  URI uri(
      StringUtil::format(
          "$0/broker/insert?topic=$1",
          server.toString(),
          URI::urlEncode(topic)));

  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addBody(record);

  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 201) {
    RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
  }
}

MessageList BrokerClient::fetch(
    const URI& server,
    const String& topic,
    size_t offset,
    size_t limit) {
  URI uri(
      StringUtil::format(
          "$0/broker/insert?topic=$1&offset=$2&limit=3",
          server.toString(),
          URI::urlEncode(topic),
          offset,
          limit));

  auto req = http::HTTPRequest::mkGet(uri);
  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 200) {
    RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
  }

  auto host_id = r.getHeader("X-Broker-HostID");
  auto msg_list = msg::decode<MessageList>(r.body());;

  for (auto& msg : *msg_list.mutable_messages()) {
    msg.set_host_id(host_id);
  }

  return msg_list;
}

//void BrokerClient::fetchCursor(
//    BrokerCursor* cursor,
//    const String& feed,
//    size_t offset,
//    size_t limit,
//    CallbackType cb);

//void BrokerClient::monitor(
//    const URI& server,
//    const String& feed,
//    CallbackType cb);

}
}
