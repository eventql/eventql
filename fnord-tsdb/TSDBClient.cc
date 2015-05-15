/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-tsdb/TSDBClient.h>
#include <fnord-base/util/BinaryMessageReader.h>

namespace fnord {
namespace tsdb {

TSDBClient::TSDBClient(
    const String& uri,
    http::HTTPConnectionPool* http) :
    uri_(uri),
    http_(http) {}

Vector<String> TSDBClient::listPartitions(
    const String& stream_key,
    const DateTime& from,
    const DateTime& until) {
  auto uri = StringUtil::format(
      "$0/list_chunks?stream=$1&from=$2&until=$3",
      uri_,
      stream_key,
      from.unixMicros(),
      until.unixMicros());

  auto req = http::HTTPRequest::mkGet(uri);
  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 200) {
    RAISEF(kRuntimeError, "received non-200 response: $0", r.body().toString());
  }

  Vector<String> partitions;
  const auto& body = r.body();
  util::BinaryMessageReader reader(body.data(), body.size());
  while (reader.remaining() > 0) {
    partitions.emplace_back(reader.readLenencString());
  }

  return partitions;
}

} // namespace tdsb
} // namespace fnord
