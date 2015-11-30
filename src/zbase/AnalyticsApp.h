/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ANALYTICSAPP_H
#define _CM_ANALYTICSAPP_H
#include "zbase/dproc/Application.h"
#include "zbase/dproc/Task.h"
#include "zbase/dproc/TaskResultFuture.h"
#include "zbase/dproc/DispatchService.h"
#include "stx/protobuf/MessageSchema.h"
#include "zbase/core/TSDBClient.h"
#include "zbase/core/TSDBService.h"
#include "zbase/core/CSTableIndex.h"
#include "zbase/ReportFactory.h"
#include "zbase/FeedConfig.pb.h"
#include "zbase/ReportParams.pb.h"
#include "zbase/api/LogfileService.h"
#include "zbase/api/EventsService.h"
#include "zbase/api/MapReduceService.h"
#include "zbase/metrics/MetricService.h"
#include "zbase/AnalyticsSession.pb.h"
#include "csql/runtime/ExecutionStrategy.h"
#include "zbase/ConfigDirectory.h"
#include <jsapi.h>

using namespace stx;

namespace zbase {

class AnalyticsApp : public dproc::DefaultApplication {
public:

  AnalyticsApp(
      zbase::TSDBService* tsdb_node,
      zbase::PartitionMap* partition_map,
      zbase::ReplicationScheme* replication_scheme,
      zbase::CSTableIndex* cstable_index,
      ConfigDirectory* cdb,
      AnalyticsAuth* auth,
      csql::Runtime* sql,
      JSRuntime* js_runtime,
      const String& datadir,
      const String& cachedir);

  RefPtr<csql::ExecutionStrategy> getExecutionStrategy(const String& customer);
  RefPtr<csql::TableProvider> getTableProvider(const String& customer) const;

  void insertMetric(
      const String& customer,
      const String& metric,
      const UnixTime& time,
      const String& value);

  // FIXPAUL remove
  void createTable(const TableDefinition& tbl);
  void updateTable(const TableDefinition& tbl, bool force = false);

  LogfileService* logfileService();
  EventsService* eventsService();
  MapReduceService* mapreduceService();
  MetricService* metricService();

protected:

  void configureCustomer(const CustomerConfig& customer);
  void configureTable(const TableDefinition& tbl);

  zbase::TSDBService* tsdb_node_;
  zbase::PartitionMap* partition_map_;
  zbase::ReplicationScheme* replication_scheme_;
  zbase::CSTableIndex* cstable_index_;
  HashMap<String, FeedConfig> feeds_;
  ConfigDirectory* cdb_;
  AnalyticsAuth* auth_;
  csql::Runtime* sql_;
  String datadir_;

  LogfileService logfile_service_;
  EventsService events_service_;
  MapReduceService mapreduce_service_;
  MetricService metric_service_;
};

zbase::TableDefinition tableDefinitionToTableConfig(const TableDefinition& tbl);

} // namespace zbase

#endif
