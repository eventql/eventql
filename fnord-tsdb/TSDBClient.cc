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
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-msg/msg.h>

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
      URI::urlEncode(stream_key),
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

void TSDBClient::fetchPartition(
    const String& stream_key,
    const String& partition,
    Function<void (const Buffer& record)> fn) {
  fetchPartitionWithSampling(stream_key, partition, 0, 0, fn);
}

void TSDBClient::fetchPartitionWithSampling(
    const String& stream_key,
    const String& partition,
    size_t sample_modulo,
    size_t sample_index,
    Function<void (const Buffer& record)> fn) {
  auto uri = StringUtil::format(
      "$0/fetch_chunk?chunk=$1",
      uri_,
      URI::urlEncode(partition));

  if (sample_modulo > 0) {
    uri += StringUtil::format("&sample=$0:$1", sample_modulo, sample_index);
  }

  Buffer buf;
  std::mutex m;
  std::condition_variable cv;
  bool done = false;
  auto handler = [&buf, &fn, &m, &cv] (const void* data, size_t size) {
    std::unique_lock<std::mutex> lk(m);
    buf.append(data, size);
    lk.unlock();
    cv.notify_all();
  };

  auto handler_factory = [&handler] (const Promise<http::HTTPResponse> promise)
      -> http::HTTPResponseFuture* {
    return new http::StreamingResponseFuture(promise, handler);
  };

  auto req = http::HTTPRequest::mkGet(uri);
  auto res = http_->executeRequest(req, handler_factory);
  res.onReady([&cv, &m, &done] () {
    std::unique_lock<std::mutex> lk(m);
    done = true;
    lk.unlock();
    cv.notify_all();
  });

  while (;;) {
    std::unique_lock<std::mutex> lk(m);

    size_t consumed = 0;
    util::BinaryMessageReader reader(buf.data(), buf.size());
    while (reader.remaining() >= sizeof(uint64_t)) {
      auto rec_len = *reader.readUInt64();

      if (rec_len > reader.remaining()) {
        break;
      }

      fn(Buffer(reader.read(rec_len), rec_len));
      consumed = reader.position();
    }

    Buffer remaining((char*) buf.data() + consumed, buf.size() - consumed);
    buf.clear();
    buf.append(remaining);

    if (done) {
      break;
    } else {
      cv.wait(lk);
    }
  }

  const auto& r = res.get();
  if (r.statusCode() != 200) {
    RAISEF(kRuntimeError, "received non-200 response: $0", r.body().toString());
  }

  return;
}

PartitionInfo TSDBClient::fetchPartitionInfo(
    const String& stream_key,
    const String& partition_key) {
  auto uri = StringUtil::format(
      "$0/fetch_partition_info?partition=$1",
      uri_,
      URI::urlEncode(partition_key));

  auto req = http::HTTPRequest::mkGet(uri);
  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 200) {
    RAISEF(kRuntimeError, "received non-200 response: $0", r.body().toString());
  }

  return msg::decode<PartitionInfo>(r.body());
}

Buffer TSDBClient::fetchDerivedDataset(
    const String& stream_key,
    const String& partition,
    const String& derived_dataset_name) {
  auto uri = StringUtil::format(
      "$0/fetch_derived_dataset?chunk=$1&derived_dataset=$2",
      uri_,
      URI::urlEncode(partition),
      URI::urlEncode(derived_dataset_name));

  auto req = http::HTTPRequest::mkGet(uri);
  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 200) {
    RAISEF(kRuntimeError, "received non-200 response: $0", r.body().toString());
  }

  return r.body();
}

} // namespace tdsb
} // namespace fnord
