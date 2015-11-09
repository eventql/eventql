/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "stx/util/Base64.h"
#include "zbase/AnalyticsApp.h"
#include "zbase/AnalyticsQueryMapper.h"
#include "zbase/AnalyticsQueryReducer.h"
#include "zbase/CSVExportRDD.h"
#include "zbase/JSONExportRDD.h"
#include "zbase/CSVSink.h"
#include "zbase/CTRCounterMergeReducer.h"
#include "zbase/ProtoSSTableMergeReducer.h"
#include "zbase/ProtoSSTableSink.h"
#include "zbase/AnalyticsSession.pb.h"
#include <zbase/core/TSDBTableScanSpec.pb.h>
#include "zbase/core/TimeWindowPartitioner.h"
#include <zbase/core/CSTableIndex.h>
#include "zbase/core/SQLEngine.h"
#include "zbase/SessionSchema.h"
#include "stx/protobuf/DynamicMessage.h"
#include "stx/protobuf/MessageEncoder.h"

using namespace stx;

namespace zbase {

AnalyticsApp::AnalyticsApp(
    zbase::TSDBService* tsdb_node,
    zbase::PartitionMap* partition_map,
    zbase::ReplicationScheme* replication_scheme,
    zbase::CSTableIndex* cstable_index,
    ConfigDirectory* cdb,
    AnalyticsAuth* auth,
    csql::Runtime* sql,
    JSRuntime* js_runtime,
    const String& datadir,
    const String& cachedir) :
    dproc::DefaultApplication("cm.analytics"),
    tsdb_node_(tsdb_node),
    partition_map_(partition_map),
    replication_scheme_(replication_scheme),
    cstable_index_(cstable_index),
    cdb_(cdb),
    auth_(auth),
    sql_(sql),
    datadir_(datadir),
    logfile_service_(
        cdb,
        auth,
        tsdb_node,
        partition_map,
        replication_scheme,
        sql),
    events_service_(
        cdb,
        auth,
        tsdb_node,
        partition_map,
        replication_scheme,
        sql),
    mapreduce_service_(
        cdb,
        auth,
        tsdb_node,
        partition_map,
        replication_scheme,
        js_runtime,
        cachedir) {
  cdb_->onCustomerConfigChange(
      std::bind(
          &AnalyticsApp::configureCustomer,
          this,
          std::placeholders::_1));

  cdb_->listCustomers([this] (const CustomerConfig& cfg) {
    configureCustomer(cfg);
  });

  cdb_->onTableDefinitionChange(
      std::bind(
          &AnalyticsApp::configureTable,
          this,
          std::placeholders::_1));

  cdb_->listTableDefinitions([this] (const TableDefinition& tbl) {
    configureTable(tbl);
  });
}

void AnalyticsApp::configureFeed(const FeedConfig& cfg) {
  feeds_.emplace(
      StringUtil::format("$0~$1", cfg.feed(), cfg.customer()),
      cfg);
}

dproc::TaskSpec AnalyticsApp::buildAnalyticsQuery(
    const AnalyticsSession& session,
    const AnalyticsQuerySpec& query_spec) {
  dproc::TaskSpec task;
  task.set_application("cm.analytics");
  task.set_task_name("AnalyticsQueryReducer");
  auto task_params = msg::encode(query_spec);
  task.set_params((char*) task_params->data(), task_params->size());
  return task;
}

dproc::TaskSpec AnalyticsApp::buildAnalyticsQuery(
    const AnalyticsSession& session,
    const URI::ParamList& params) {
  AnalyticsQuerySpec q;
  AnalyticsSubQuerySpec* subq = nullptr;

  q.set_customer(session.customer());
  q.set_end_time(WallClock::unixMicros());
  q.set_start_time(q.end_time() - 30 * kMicrosPerDay);

  for (const auto& p : params) {
    if (p.first == "from") {
      if (p.second.length() > 0) {
        q.set_start_time(std::stoul(p.second) * kMicrosPerSecond);
      }
      continue;
    }

    if (p.first == "until") {
      if (p.second.length() > 0) {
        q.set_end_time(std::stoul(p.second) * kMicrosPerSecond);
      }
      continue;
    }

    if (p.first == "query") {
      subq = q.add_subqueries();
      subq->set_query(p.second);
      continue;
    }

    if (p.first == "segments") {
      if (p.second.length() == 0) {
        continue;
      }

      String segments_json;
      util::Base64::decode(p.second, &segments_json);
      q.set_segments(segments_json);
      continue;
    }

    if (subq != nullptr) {
      auto param = subq->add_params();
      param->set_key(p.first);
      param->set_value(p.second);
    }
  }

  q.set_start_time((q.start_time() / kMicrosPerDay) * kMicrosPerDay);
  q.set_end_time(((q.end_time() / kMicrosPerDay) + 1) * kMicrosPerDay);

  return buildAnalyticsQuery(session, q);
}

dproc::TaskSpec AnalyticsApp::buildReportQuery(
      const String& customer,
      const String& report_name,
      const UnixTime& from,
      const UnixTime& until,
      const URI::ParamList& params) {
  ReportParams report;
  report.set_report(report_name);
  report.set_customer(customer);
  report.set_from_unixmicros(from.unixMicros());
  report.set_until_unixmicros(until.unixMicros());
  report.set_params(URI::buildQueryString(params));

  dproc::TaskSpec task;
  task.set_application("cm.analytics");
  task.set_task_name(report_name);
  task.set_params(msg::encode(report)->toString());

  return task;
}

dproc::TaskSpec AnalyticsApp::buildFeedQuery(
      const String& customer,
      const String& feed,
      uint64_t sequence) {
  auto cfg_iter = feeds_.find(
      StringUtil::format(
          "$0~$1",
          feed,
          customer));

  if (cfg_iter == feeds_.end()) {
    RAISEF(
        kRuntimeError,
        "feed '$0' not configured for customer '$1'",
        feed,
        customer);
  }

  const auto& cfg = cfg_iter->second;
  auto ts =
      cfg.first_partition() +
      cfg.partition_size() * (sequence / cfg.num_shards());

  auto table_name = cfg.table_name();
  auto partition_key = zbase::TimeWindowPartitioner::partitionKeyFor(
      table_name,
      ts,
      cfg.partition_size());

  zbase::TSDBTableScanSpec params;
  params.set_tsdb_namespace(customer);
  params.set_table_name(table_name);
  params.set_partition_key(partition_key.toString());
  params.set_sample_modulo(cfg.num_shards());
  params.set_sample_index(sequence % cfg.num_shards());

  auto params_buf = msg::encode(params);

  dproc::TaskSpec task;
  task.set_application(name());
  task.set_task_name(feed);
  task.set_params((char*) params_buf->data(), params_buf->size());

  return task;
}

RefPtr<csql::ExecutionStrategy> AnalyticsApp::getExecutionStrategy(
    const String& customer) {
  return SQLEngine::getExecutionStrategy(
      sql_,
      partition_map_,
      replication_scheme_,
      cstable_index_,
      auth_,
      customer);
}

RefPtr<csql::TableProvider> AnalyticsApp::getTableProvider(
    const String& customer) const {
  return zbase::SQLEngine::tableProviderForNamespace(
        partition_map_,
        replication_scheme_,
        cstable_index_,
        auth_,
        customer);
}

void AnalyticsApp::createTable(const TableDefinition& tbl) {
  cdb_->updateTableDefinition(tbl);
}

void AnalyticsApp::updateTable(const TableDefinition& tbl, bool force) {
  cdb_->updateTableDefinition(tbl, force);
}

void AnalyticsApp::configureTable(const TableDefinition& tbl) {
  tsdb_node_->createTable(tbl);
}

void AnalyticsApp::configureCustomer(const CustomerConfig& config) {
  for (const auto& td : SessionSchema::tableDefinitionsForCustomer(config)) {
    configureTable(td);
  }

  for (const auto& td : logfile_service_.getTableDefinitions(config)) {
    configureTable(td);
  }
}

void AnalyticsApp::insertMetric(
    const String& customer,
    const String& metric,
    const UnixTime& time,
    const String& value) {
  RefPtr<zbase::Table> table;
  auto table_opt = partition_map_->findTable(customer, metric);
  if (table_opt.isEmpty()) {
    auto schema = msg::MessageSchema(
        "generic_metric",
        Vector<msg::MessageSchemaField> {
            msg::MessageSchemaField(
                1,
                "time",
                msg::FieldType::DATETIME,
                0,
                false,
                true),
            msg::MessageSchemaField(
                2,
                "value",
                msg::FieldType::DOUBLE,
                0,
                false,
                true),
        });

    TableDefinition td;
    td.set_customer(customer);
    td.set_table_name(metric);
    auto tblcfg = td.mutable_config();
    tblcfg->set_schema(schema.encode().toString());
    tblcfg->set_partitioner(zbase::TBL_PARTITION_TIMEWINDOW);
    tblcfg->set_storage(zbase::TBL_STORAGE_LOG);
    createTable(td);
    table = partition_map_->findTable(customer, metric).get();
  } else {
    table = table_opt.get();
  }

  msg::DynamicMessage smpl(table->schema());
  smpl.addField("time", StringUtil::toString(time));
  smpl.addField("value", value);

  Buffer smpl_buf;
  msg::MessageEncoder::encode(smpl.data(), *smpl.schema(), &smpl_buf);

  tsdb_node_->insertRecord(
      customer,
      metric,
      zbase::TimeWindowPartitioner::partitionKeyFor(
          metric,
          time,
          table->partitionSize()),
      Random::singleton()->sha1(),
      smpl_buf);
}

LogfileService* AnalyticsApp::logfileService() {
  return &logfile_service_;
}

EventsService* AnalyticsApp::eventsService() {
  return &events_service_;
}

MapReduceService* AnalyticsApp::mapreduceService() {
  return &mapreduce_service_;
}

} // namespace zbase
