/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <unistd.h>
#include "eventql/transport/http/EventsService.h"
#include "eventql/util/RegExp.h"
#include "eventql/util/human.h"
#include "eventql/util/protobuf/msg.h"
#include "eventql/util/protobuf/MessageSchema.h"
#include "eventql/util/protobuf/MessagePrinter.h"
#include "eventql/util/protobuf/MessageEncoder.h"
#include "eventql/util/protobuf/MessageDecoder.h"
#include "eventql/util/protobuf/DynamicMessage.h"
#include "eventql/sql/qtree/SelectListNode.h"
#include "eventql/sql/qtree/ColumnReferenceNode.h"
#include "eventql/sql/CSTableScan.h"
#include "eventql/db/TimeWindowPartitioner.h"

#include "eventql/eventql.h"

namespace eventql {

EventsService::EventsService(
    ConfigDirectory* cdir,
    InternalAuth* auth,
    eventql::TableService* tsdb,
    eventql::PartitionMap* pmap,
    eventql::ReplicationScheme* repl,
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
  auto table_cfg = table.get()->config().config();

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

    auto partition = eventql::TimeWindowPartitioner::partitionKeyFor(
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
      "eventql",
      "Scanning local table partition $0/$1/$2",
      session.customer(),
      table_name,
      partition_key.toString());

  auto reader = partition.get()->getReader();
  auto schema = table.get()->schema();
  auto time_field_id = schema->fieldId("time");

  size_t nrows = 0;
  reader->fetchRecords(
      Set<String>{},
      [
        &result,
        &nrows,
        &schema,
        time_field_id
      ] (const msg::MessageObject& record) {
    auto time = record.getUnixTime(time_field_id);
    auto row = result->addRow(time);
    if (row) {
      row->obj.setData(record);
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
    const Vector<String>& hosts,
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
          "eventql",
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
    const String& host,
    EventScanResult* result) {
  logDebug(
      "eventql",
      "Scanning remote table partition $0/$1/$2 on $3",
      session.customer(),
      table_name,
      partition_key.toString(),
      host);

  auto url = StringUtil::format(
      "http://$0/api/v1/events/scan_partition?table=$1&partition=$2&limit=$3",
      host,
      URI::urlEncode(table_name),
      partition_key.toString(),
      result->capacity());

  http::HTTPClient http_client(&evqld_stats()->http_client_stats);
  auto req_body = msg::encode(params);
  auto req = http::HTTPRequest::mkPost(url, *req_body);
  //auth_->signRequest(session_, &req);
  RAISE(kNotImplementedError);

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

} // namespace eventql
