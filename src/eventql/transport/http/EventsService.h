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
#pragma once
#include "eventql/util/protobuf/MessageSchema.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/db/TSDBService.h"
#include "eventql/AnalyticsAuth.h"
#include "eventql/CustomerConfig.h"
#include "eventql/config/config_directory.h"
#include "eventql/TableDefinition.h"
#include "eventql/EventScanResult.h"
#include "eventql/AnalyticsSession.pb.h"
#include "eventql/EventScanParams.pb.h"
#include "eventql/sql/runtime/runtime.h"

#include "eventql/eventql.h"

namespace eventql {

class EventsService {
public:

  EventsService(
      ConfigDirectory* cdir,
      AnalyticsAuth* auth,
      eventql::TSDBService* tsdb,
      eventql::PartitionMap* pmap,
      eventql::ReplicationScheme* repl,
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
  eventql::TSDBService* tsdb_;
  eventql::PartitionMap* pmap_;
  eventql::ReplicationScheme* repl_;
  csql::Runtime* sql_;
};

} // namespace eventql
