/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/protobuf/MessageSchema.h"
#include "stx/io/inputstream.h"
#include "zbase/core/TSDBService.h"
#include "zbase/AnalyticsAuth.h"
#include "zbase/CustomerConfig.h"
#include "zbase/ConfigDirectory.h"
#include "zbase/TableDefinition.h"
#include "zbase/EventScanResult.h"
#include "zbase/AnalyticsSession.pb.h"
#include "zbase/EventScanParams.pb.h"
#include "csql/runtime/runtime.h"

using namespace stx;

namespace zbase {

class EventsService {
public:

  EventsService(
      ConfigDirectory* cdir,
      AnalyticsAuth* auth,
      zbase::TSDBService* tsdb,
      zbase::PartitionMap* pmap,
      zbase::ReplicationScheme* repl,
      csql::Runtime* sql);

  /**
   * Scan a table, returns the "limit" newest rows that match the condition
   * and are older than end_time
   */
  void insertRow(
      const AnalyticsSession& session,
      const String& table_name,
      const json::JSONObject::const_iterator& data_begin,
      const json::JSONObject::const_iterator& data_end);

  /**
   * Scan a table, returns the "limit" newest rows that match the condition
   * and are older than end_time
   */
  void scanTable(
      const AnalyticsSession& session,
      const String& table_name,
      const EventScanParams& params,
      Function<void (const msg::DynamicMessage& event)> on_row,
      Function<void (bool done)> on_progress);

  /**
   * Scan a single table partition. This method must be executed on a host
   * that actually stores this partition
   */
  void scanLocalTablePartition(
      const AnalyticsSession& session,
      const String& table_name,
      const SHA1Hash& partition,
      const EventScanParams& params,
      EventScanResult* result);

  /**
   * Scan a single table partition. This method must be executed on a host
   * that actually stores this partition
   */
  EventScanResult scanLocalTablePartition(
      const AnalyticsSession& session,
      const String& table_name,
      const SHA1Hash& partition,
      const EventScanParams& params);

  /**
   * Scan a single remote table partition, try all hosts in order from first
   * to last
   */
  void scanRemoteTablePartition(
      const AnalyticsSession& session,
      const String& table_name,
      const SHA1Hash& partition,
      const EventScanParams& params,
      const Vector<InetAddr>& hosts,
      EventScanResult* result);

  /**
   * Scan a single remote table partition on a specific host
   */
  bool scanRemoteTablePartition(
      const AnalyticsSession& session,
      const String& table_name,
      const SHA1Hash& partition,
      const EventScanParams& params,
      const InetAddr& hosts,
      EventScanResult* result);

protected:
  ConfigDirectory* cdir_;
  AnalyticsAuth* auth_;
  zbase::TSDBService* tsdb_;
  zbase::PartitionMap* pmap_;
  zbase::ReplicationScheme* repl_;
  csql::Runtime* sql_;
};

} // namespace zbase
