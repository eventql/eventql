/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <brokerd/BrokerClient.h>
#include <stx/protobuf/msg.h>

namespace stx {
namespace feeds {

BrokerClient::BrokerClient(http::HTTPConnectionPool* http) : http_(http) {}

void BrokerClient::insert(
    const InetAddr& server,
    const String& topic,
    const Buffer& record) {
  URI uri(
      StringUtil::format(
          "http://$0/broker/insert?topic=$1",
          server.hostAndPort(),
          URI::urlEncode(topic)));

  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addBody(record);

  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 201) {
    RAISEF(
        kRuntimeError,
        "received non-200 response: for '$0': $1",
        uri.toString(),
        r.body().toString());
  }
}

MessageList BrokerClient::fetch(
    const InetAddr& server,
    const String& topic,
    size_t offset,
    size_t limit) {
  URI uri(
      StringUtil::format(
          "http://$0/broker/fetch?topic=$1&offset=$2&limit=$3",
          server.hostAndPort(),
          URI::urlEncode(topic),
          offset,
          limit));

  auto req = http::HTTPRequest::mkGet(uri);
  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 200) {
    RAISEF(
        kRuntimeError,
        "received non-200 response: for '$0': $1",
        uri.toString(),
        r.body().toString());
  }

  auto host_id = r.getHeader("X-Broker-HostID");
  auto msg_list = msg::decode<MessageList>(r.body());;

  for (auto& msg : *msg_list.mutable_messages()) {
    msg.set_host_id(host_id);
  }

  return msg_list;
}

MessageList BrokerClient::fetchNext(
    const InetAddr& server,
    TopicCursor* cursor,
    size_t batch_size) {
  auto host_id = hostID(server);

  TopicCursorOffset* offset = nullptr;
  for (auto& o : *cursor->mutable_offsets()) {
    if (o.host_id() == host_id) {
      offset = &o;
      break;
    }
  }

  if (offset == nullptr) {
    offset = cursor->add_offsets();
    offset->set_host_id(host_id);
  }

  auto msg_list = fetch(
      server,
      cursor->topic(),
      offset->next_offset(),
      batch_size);

  for (const auto& msg : msg_list.messages()) {
    if (msg.next_offset() > offset->next_offset()) {
      offset->set_next_offset(msg.next_offset());
    }
  }

  return msg_list;
}


String BrokerClient::hostID(const InetAddr& server) {
  auto uri = "http://" + server.hostAndPort() + "/broker/host_id";
  auto req = http::HTTPRequest::mkGet(uri);
  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 200) {
    RAISEF(
        kRuntimeError,
        "received non-200 response: for '$0': $1",
        uri,
        r.body().toString());
  }

  return r.body().toString();
}

}
}
