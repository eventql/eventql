/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <zbase/core/TSDBClient.h>
#include <zbase/core/RecordEnvelope.pb.h>
#include <stx/util/binarymessagereader.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/protobuf/msg.h>
#include <stx/http/httpclient.h>
#include <stx/logging.h>
#include <zbase/z1stats.h>

using namespace stx;

namespace zbase {

TSDBClient::TSDBClient(
    const String& uri,
    http::HTTPConnectionPool* http) :
    uri_(uri),
    http_(http) {}

void TSDBClient::insertRecord(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    const SHA1Hash& record_id,
    const Buffer& record_data) {
  RecordEnvelopeList records;

  auto record = records.add_records();
  record->set_tsdb_namespace(tsdb_namespace);
  record->set_table_name(table_name);
  record->set_partition_sha1(partition_key.toString());
  record->set_record_id(record_id.toString());
  record->set_record_data(record_data.toString());

  insertRecords(records);
}

void TSDBClient::insertRecord(const RecordEnvelope& record) {
  RecordEnvelopeList records;
  *records.add_records() = record;
  insertRecords(records);
}

void TSDBClient::insertRecords(const RecordEnvelopeList& records) {
  HashMap<String, List<RecordEnvelopeList>> batches;

  for (const auto& r : records.records()) {
    auto& list = batches[uri_];

    if (list.empty() || list.back().records().size() > kMaxInsertBachSize) {
      list.emplace_back();
    }

    *list.back().add_records() = r;
  }

  for (const auto& per_host : batches) {
    for (const auto& b : per_host.second) {
      insertRecordsToHost(per_host.first, b);
    }
  }
}

void TSDBClient::insertRecordsToHost(
    const String& host,
    const RecordEnvelopeList& records) {
  auto uri = URI(StringUtil::format("$0/insert", host));

  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addBody(*msg::encode(records));

  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 201) {
    RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
  }
}

//Vector<String> TSDBClient::listPartitions(
//    const String& table_name,
//    const UnixTime& from,
//    const UnixTime& until) {
//  auto uri = StringUtil::format(
//      "$0/list_chunks?stream=$1&from=$2&until=$3",
//      uri_,
//      URI::urlEncode(table_name),
//      from.unixMicros(),
//      until.unixMicros());
//
//  auto req = http::HTTPRequest::mkGet(uri);
//  auto res = http_->executeRequest(req);
//  res.wait();
//
//  const auto& r = res.get();
//  if (r.statusCode() != 200) {
//    RAISEF(kRuntimeError, "received non-200 response: $0", r.body().toString());
//  }
//
//  Vector<String> partitions;
//  const auto& body = r.body();
//  util::BinaryMessageReader reader(body.data(), body.size());
//  while (reader.remaining() > 0) {
//    partitions.emplace_back(reader.readLenencString());
//  }
//
//  return partitions;
//}

void TSDBClient::fetchPartition(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    Function<void (const Buffer& record)> fn) {
  fetchPartitionWithSampling(
      tsdb_namespace,
      table_name,
      partition_key,
      0,
      0,
      fn);
}

void TSDBClient::fetchPartitionWithSampling(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key,
    size_t sample_modulo,
    size_t sample_index,
    Function<void (const Buffer& record)> fn) {
  http::HTTPClient http(&z1stats()->http_client_stats);

  auto uri = StringUtil::format(
      "$0/stream?namespace=$1&stream=$2&partition=$3",
      uri_,
      URI::urlEncode(tsdb_namespace),
      URI::urlEncode(table_name),
      partition_key.toString());

  if (sample_modulo > 0) {
    uri += StringUtil::format("&sample=$0:$1", sample_modulo, sample_index);
  }

  Buffer buffer;
  bool eos = false;

  auto handler = [&buffer, &fn, &eos] (const void* data, size_t size) {
    buffer.append(data, size);
    size_t consumed = 0;

    util::BinaryMessageReader reader(buffer.data(), buffer.size());
    while (reader.remaining() >= sizeof(uint64_t)) {
      auto rec_len = *reader.readUInt64();

      if (rec_len > reader.remaining()) {
        break;
      }

      if (rec_len == 0) {
        eos = true;
      } else {
        fn(Buffer(reader.read(rec_len), rec_len));
        consumed = reader.position();
      }
    }

    Buffer remaining((char*) buffer.data() + consumed, buffer.size() - consumed);
    buffer.clear();
    buffer.append(remaining);
  };

  auto handler_factory = [&handler] (const Promise<http::HTTPResponse> promise)
      -> http::HTTPResponseFuture* {
    return new http::StreamingResponseFuture(promise, handler);
  };

  stx::logTrace("tsdb.client", "Executing request: $0", uri);

  auto req = http::HTTPRequest::mkGet(uri);
  auto res = http.executeRequest(req, handler_factory);

  handler(nullptr, 0);

  if (!eos) {
    RAISE(kRuntimeError, "unexpected EOF");
  }

  if (res.statusCode() != 200) {
    RAISEF(
        kRuntimeError,
        "received non-200 response: $0",
        res.body().toString());
  }
}

Option<PartitionInfo> TSDBClient::partitionInfo(
    const String& tsdb_namespace,
    const String& table_name,
    const SHA1Hash& partition_key) {
  auto uri = StringUtil::format(
      "$0/partition_info?namespace=$1&stream=$2&partition=$3",
      uri_,
      URI::urlEncode(tsdb_namespace),
      URI::urlEncode(table_name),
      partition_key.toString());

  auto req = http::HTTPRequest::mkGet(uri);
  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() == 404) {
    return None<PartitionInfo>();
  }

  if (r.statusCode() != 200) {
    RAISEF(kRuntimeError, "received non-200 response: $0", r.body().toString());
  }

  return Some(msg::decode<PartitionInfo>(r.body()));
}

//Buffer TSDBClient::fetchDerivedDataset(
//    const String& table_name,
//    const String& partition,
//    const String& derived_dataset_name) {
//  auto uri = StringUtil::format(
//      "$0/fetch_derived_dataset?chunk=$1&derived_dataset=$2",
//      uri_,
//      URI::urlEncode(partition),
//      URI::urlEncode(derived_dataset_name));
//
//  auto req = http::HTTPRequest::mkGet(uri);
//  auto res = http_->executeRequest(req);
//  res.wait();
//
//  const auto& r = res.get();
//  if (r.statusCode() != 200) {
//    RAISEF(kRuntimeError, "received non-200 response: $0", r.body().toString());
//  }
//
//  return r.body();
//}
//
uint64_t TSDBClient::mkMessageID() {
  return rnd_.random64();
}

} // namespace tdsb
