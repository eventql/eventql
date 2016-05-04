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
#include "eventql/server/sql/sql_engine.h"
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
      auth_,
      customer);
}

RefPtr<csql::TableProvider> AnalyticsApp::getTableProvider(
    const String& customer) const {
  return zbase::SQLEngine::tableProviderForNamespace(
        partition_map_,
        replication_scheme_,
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
  for (const auto& td : logfile_service_.getTableDefinitions(config)) {
    configureTable(td);
  }
}

void AnalyticsApp::insertMetric(
    const String& customer,
    const String& metric,
    const UnixTime& time,
    const String& value) {}

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
