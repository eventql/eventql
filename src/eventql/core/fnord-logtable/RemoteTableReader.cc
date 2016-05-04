/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-logtable/RemoteTableReader.h>
#include <eventql/util/protobuf/MessageDecoder.h>

namespace stx {
namespace logtable {

RemoteTableReader::RemoteTableReader(
    const String& table_name,
    const msg::MessageSchema& schema,
    const URI& uri,
    http::HTTPConnectionPool* http) :
    name_(table_name),
    schema_(schema),
    uri_(uri),
    http_(http) {}

const String& RemoteTableReader::name() const {
  return name_;
}

const msg::MessageSchema& RemoteTableReader::schema() const {
  return schema_;
}

RefPtr<TableSnapshot> RemoteTableReader::getSnapshot() {
  auto path = StringUtil::format("/snapshot?table=$0", name_);
  URI uri(uri_.toString() + path);

  http::HTTPRequest req(http::HTTPMessage::M_GET, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());

  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 200) {
    RAISEF(kRuntimeError, "received non-200 response: $0", r.body().toString());
  }

  RefPtr<TableSnapshot> snap(new TableSnapshot());
  snap->head->decode(r.body());
  return snap;
}

size_t RemoteTableReader::fetchRecords(
    const String& replica,
    uint64_t start_sequence,
    size_t limit,
    Function<bool (const msg::MessageObject& record)> fn) {
  auto path = StringUtil::format(
      "/fetch_batch?table=$0&replica=$1&seq=$2&limit=$3",
      name_,
      replica,
      start_sequence,
      limit);

  URI uri(uri_.toString() + path);

  http::HTTPRequest req(http::HTTPMessage::M_GET, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addHeader("Content-Type", "application/fnord-msg");

  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 200) {
    RAISEF(kRuntimeError, "received non-200 response: $0", r.body().toString());
  }

  size_t n = 0;
  const auto& buf = r.body();
  util::BinaryMessageReader reader(buf.data(), buf.size());
  while (reader.remaining() > 0) {
    auto len = reader.readVarUInt();

    msg::MessageObject msg;
    msg::MessageDecoder::decode(reader.read(len), len, schema_, &msg);

    ++n;
    if (!fn(msg)) {
      break;
    }
  }

  return n;
}

} // namespace logtable
} // namespace stx


