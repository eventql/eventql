/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <fnord-logtable/RemoteTableReader.h>
#include <eventql/util/protobuf/MessageDecoder.h>

namespace util {
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
} // namespace util


