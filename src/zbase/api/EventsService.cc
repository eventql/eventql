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
    EventScanResult* result,
    Function<void (bool done)> on_progress) {
  /*
  auto table_definition = findEventsDefinition(
      session.customer(), table_name);
  if (table_definition.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", table_name);
  }

  auto table_name = "logs." + table_name;
  auto lookback_limit = params.end_time() - 90 * kMicrosPerDay;
  auto partition_size = 10 * kMicrosPerMinute;

  result->setColumns(
      Vector<String>(params.columns().begin(), params.columns().end()));

  for (auto time = params.end_time();
      time > lookback_limit;
      time -= partition_size) {

    auto partition = zbase::TimeWindowPartitioner::partitionKeyFor(
        table_name,
        time,
        partition_size);

    if (repl_->hasLocalReplica(partition)) {
      scanLocalEventsPartition(
          session,
          table_name,
          partition,
          params,
          result);
    } else {
      scanRemoteTablePartition(
          session,
          table_name,
          partition,
          params,
          repl_->replicaAddrsFor(partition),
          result);
    }

    result->setScannedUntil(time);

    bool done = result->isFull();
    on_progress(done);

    if (done) {
      break;
    }
  }
  */
}

void EventsService::scanLocalTablePartition(
    const AnalyticsSession& session,
    const String& table_name,
    const SHA1Hash& partition_key,
    const EventScanParams& params,
    EventScanResult* result) {
  /*
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

  Vector<RefPtr<csql::SelectListNode>> select_list;
  select_list.emplace_back(
      new csql::SelectListNode(
          new csql::ColumnReferenceNode("time")));

  if (params.return_raw()) {
    select_list.emplace_back(
        new csql::SelectListNode(
            new csql::ColumnReferenceNode("raw")));
  }

  for (const auto& c : params.columns()) {
    select_list.emplace_back(
        new csql::SelectListNode(
            new csql::ColumnReferenceNode(c)));
  }

  Option<RefPtr<csql::ValueExpressionNode>> where_cond;
  switch (params.scan_type()) {

    case LOGSCAN_SQL: {
      const auto& sql_str = params.condition();

      csql::Parser parser;
      parser.parseValueExpression(sql_str.data(), sql_str.size());

      auto stmts = parser.getStatements();
      if (stmts.size() != 1) {
        RAISE(
            kParseError,
            "SQL filter expression must consist of exactly one statement");
      }

      where_cond = Some(
          mkRef(sql_->queryPlanBuilder()->buildValueExpression(stmts[0])));

      break;
    }

  }

  auto seqscan = mkRef(
      new csql::SequentialScanNode(
          table_name,
          select_list,
          where_cond));

  auto reader = partition.get()->getReader();
  auto cstable_filename = reader->cstableFilename();
  if (cstable_filename.isEmpty()) {
    return;
  }

  csql::CSTableScan cstable_scan(
      seqscan,
      cstable_filename.get(),
      sql_->queryBuilder().get());

  csql::ExecutionContext context(sql_->scheduler());

  cstable_scan.execute(
      &context,
      [result, &params] (int argc, const csql::SValue* argv) -> bool {
    int colidx = 0;

    auto time = UnixTime(argv[colidx++].getInteger());
    if (time >= params.end_time()) {
      return true;
    }

    auto line = result->addLine(time);
    if (!line) {
      return true;
    }

    if (params.return_raw()) {
      line->raw = argv[colidx++].toString();
    }

    for (; colidx < argc; ++colidx) {
      line->columns.emplace_back(argv[colidx].toString());
    }

    return true;
  });

  result->incrRowsScanned(cstable_scan.rowsScanned());
  */
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
