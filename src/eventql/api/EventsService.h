/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "eventql/util/protobuf/MessageSchema.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/core/TSDBService.h"
#include "eventql/AnalyticsAuth.h"
#include "eventql/CustomerConfig.h"
#include "eventql/ConfigDirectory.h"
#include "eventql/TableDefinition.h"
#include "eventql/EventScanResult.h"
#include "eventql/AnalyticsSession.pb.h"
#include "eventql/EventScanParams.pb.h"
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
