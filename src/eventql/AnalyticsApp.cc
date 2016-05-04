/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/util/wallclock.h"
#include "eventql/util/util/Base64.h"
#include "eventql/AnalyticsApp.h"
#include "eventql/AnalyticsSession.pb.h"
#include <eventql/core/TSDBTableScanSpec.pb.h>
#include "eventql/core/TimeWindowPartitioner.h"
#include <eventql/core/CompactionWorker.h>
#include "eventql/core/SQLEngine.h"
#include "eventql/SessionSchema.h"
#include "eventql/util/protobuf/DynamicMessage.h"
#include "eventql/util/protobuf/MessageEncoder.h"

using namespace stx;

namespace zbase {

AnalyticsApp::AnalyticsApp(
    zbase::TSDBService* tsdb_node,
    zbase::PartitionMap* partition_map,
    zbase::ReplicationScheme* replication_scheme,
    zbase::CompactionWorker* cstable_index,
    ConfigDirectory* cdb,
    AnalyticsAuth* auth,
    csql::Runtime* sql,
    JSRuntime* js_runtime,
    const String& datadir,
    const String& cachedir) :
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
        cachedir),
    metric_service_(
        cdb,
        auth,
        tsdb_node,
        partition_map,
        replication_scheme,
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

zbase::TSDBService* AnalyticsApp::getTSDBNode() const {
  return tsdb_node_;
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
    tblcfg->set_storage(zbase::TBL_STORAGE_COLSM);
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
      WallClock::unixMicros(),
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

MetricService* AnalyticsApp::metricService() {
  return &metric_service_;
}

} // namespace zbase
