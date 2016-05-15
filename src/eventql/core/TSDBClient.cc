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
#include <eventql/core/TSDBClient.h>
#include <eventql/core/RecordEnvelope.pb.h>
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/util/http/httpclient.h>
#include <eventql/util/logging.h>
#include <eventql/z1stats.h>

#include "eventql/eventql.h"

namespace eventql {

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

  util::logTrace("tsdb.client", "Executing request: $0", uri);

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
