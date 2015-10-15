/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <unistd.h>
#include "zbase/api/EventsService.h"
#include "stx/RegExp.h"
#include "stx/human.h"
#include "stx/protobuf/msg.h"
#include "stx/protobuf/MessageSchema.h"
#include "stx/protobuf/MessagePrinter.h"
#include "stx/protobuf/MessageEncoder.h"
#include "stx/protobuf/MessageDecoder.h"
#include "stx/protobuf/DynamicMessage.h"
#include "csql/qtree/SelectListNode.h"
#include "csql/qtree/ColumnReferenceNode.h"
#include "csql/CSTableScan.h"
#include "zbase/core/TimeWindowPartitioner.h"

using namespace stx;

namespace zbase {

EventsService::EventsService(
    ConfigDirectory* cdir,
    AnalyticsAuth* auth,
    zbase::TSDBService* tsdb,
    zbase::PartitionMap* pmap,
    zbase::ReplicationScheme* repl,
    csql::Runtime* sql) :
    cdir_(cdir),
    auth_(auth),
    tsdb_(tsdb),
    pmap_(pmap),
    repl_(repl),
    sql_(sql) {}

void EventsService::scanTable(
    const AnalyticsSession& session,
    const String& table_name,
    const EventScanParams& params,
    Function<void (const msg::DynamicMessage& event)> on_row,
    Function<void (bool done)> on_progress) {

  auto table = pmap_->findTable(session.customer(), table_name);
  if (table.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", table_name);
  }

  if (table.get()->partitionerType() != TBL_PARTITION_TIMEWINDOW) {
    RAISEF(kNotFoundError, "table '$0' is not a  timeseries", table_name);
  }

  auto schema = table.get()->schema();
  const auto& table_cfg = table.get()->config().config();

  if (!schema->hasField("time")) {
    RAISEF(kNotFoundError, "table '$0' has no 'time' column", table_name);
  }

  auto lookback_limit = params.end_time() - 90 * kMicrosPerDay;
  auto partition_size =
      table_cfg.time_window_partitioner_config().partition_size();

  if (!partition_size) {
    partition_size = 4 * kMicrosPerHour;
  }

  size_t remaining = params.limit();
  for (auto time = params.end_time();
      time > lookback_limit;
      time -= partition_size) {

    EventScanResult result(schema, remaining);

    auto partition = zbase::TimeWindowPartitioner::partitionKeyFor(
        table_name,
        time,
        partition_size);

    if (repl_->hasLocalReplica(partition)) {
      scanLocalTablePartition(
          session,
          table_name,
          partition,
          params,
          &result);
    } else {
      scanRemoteTablePartition(
          session,
          table_name,
          partition,
          params,
          repl_->replicaAddrsFor(partition),
          &result);
    }

    auto num_result_rows = result.rows().size();
    if (num_result_rows > remaining) {
      RAISE(kIllegalStateError);
    }

    for (const auto& r : result.rows()) {
      on_row(r.obj);
    }

    remaining -= num_result_rows;

    on_progress(remaining == 0);

    if (remaining == 0) {
      break;
    }
  }
}

EventScanResult EventsService::scanLocalTablePartition(
    const AnalyticsSession& session,
    const String& table_name,
    const SHA1Hash& partition_key,
    const EventScanParams& params) {
  auto table = pmap_->findTable(session.customer(), table_name);
  if (table.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", table_name);
  }

  EventScanResult result(table.get()->schema(), params.limit());
  scanLocalTablePartition(
      session,
      table_name,
      partition_key,
      params,
      &result);

  return result;
}

void EventsService::scanLocalTablePartition(
    const AnalyticsSession& session,
    const String& table_name,
    const SHA1Hash& partition_key,
    const EventScanParams& params,
    EventScanResult* result) {
  auto table = pmap_->findTable(session.customer(), table_name);
  if (table.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", table_name);
  }

  auto partition = pmap_->findPartition(
      session.customer(),
      table_name,
      partition_key);

  if (partition.isEmpty()) {
    return;
  }

  logDebug(
      "zbase",
      "Scanning local table partition $0/$1/$2",
      session.customer(),
      table_name,
      partition_key.toString());

  auto reader = partition.get()->getReader();
  auto schema = table.get()->schema();
  auto time_field_id = schema->fieldId("time");

  size_t nrows = 0;
  reader->fetchRecords(
      [&result, &nrows, &schema, time_field_id] (const Buffer& record) {

    msg::MessageObject msg;
    msg::MessageDecoder::decode(record, *schema, &msg);

    auto time = msg.getUnixTime(time_field_id);
    auto row = result->addRow(time);
    if (row) {
      row->obj.setData(msg);
    }

    if (++nrows % 10000 == 0) {
      result->incrRowsScanned(nrows);
      nrows = 0;
    }
  });

  result->incrRowsScanned(nrows);
}

void EventsService::scanRemoteTablePartition(
    const AnalyticsSession& session,
    const String& table_name,
    const SHA1Hash& partition_key,
    const EventScanParams& params,
    const Vector<InetAddr>& hosts,
    EventScanResult* result) {
  Vector<String> errors;

  for (const auto& host : hosts) {
    try {
      if(scanRemoteTablePartition(
          session,
          table_name,
          partition_key,
          params,
          host,
          result)) {
        return;
      }
    } catch (const StandardException& e) {
      logError(
          "zbase",
          e,
          "EventsService::scanRemoteTablePartition failed");

      errors.emplace_back(e.what());
    }
  }

  if (!errors.empty()) {
    RAISEF(
        kRuntimeError,
        "EventsService::scanRemoteTablePartition failed: $0",
        StringUtil::join(errors, ", "));
  }
}

bool EventsService::scanRemoteTablePartition(
    const AnalyticsSession& session,
    const String& table_name,
    const SHA1Hash& partition_key,
    const EventScanParams& params,
    const InetAddr& host,
    EventScanResult* result) {
  logDebug(
      "zbase",
      "Scanning remote table partition $0/$1/$2 on $3",
      session.customer(),
      table_name,
      partition_key.toString(),
      host.hostAndPort());

  auto url = StringUtil::format(
      "http://$0/api/v1/events/scan_partition?table=$1&partition=$2&limit=$3",
      host.hostAndPort(),
      URI::urlEncode(table_name),
      partition_key.toString(),
      result->capacity());

  auto api_token = auth_->encodeAuthToken(session);

  http::HTTPMessage::HeaderList auth_headers;
  auth_headers.emplace_back(
      "Authorization",
      StringUtil::format("Token $0", api_token));

  http::HTTPClient http_client;
  auto req_body = msg::encode(params);
  auto req = http::HTTPRequest::mkPost(url, *req_body, auth_headers);
  auto res = http_client.executeRequest(req);

  if (res.statusCode() == 404) {
    return false;
  }

  if (res.statusCode() != 200) {
    RAISEF(
        kRuntimeError,
        "received non-200 response: $0", res.body().toString());
  }

  auto body_is = BufferInputStream::fromBuffer(&res.body());
  result->decode(body_is.get());

  return true;
}

} // namespace zbase
