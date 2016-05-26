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
#include "eventql/util/wallclock.h"
#include "eventql/util/util/Base64.h"
#include "eventql/AnalyticsApp.h"
#include "eventql/AnalyticsSession.pb.h"
#include <eventql/db/TSDBTableScanSpec.pb.h>
#include "eventql/db/TimeWindowPartitioner.h"
#include <eventql/db/CompactionWorker.h>
#include "eventql/server/sql/sql_engine.h"
#include "eventql/util/protobuf/DynamicMessage.h"
#include "eventql/util/protobuf/MessageEncoder.h"

#include "eventql/eventql.h"

namespace eventql {

AnalyticsApp::AnalyticsApp(
    eventql::TSDBService* tsdb_node,
    eventql::PartitionMap* partition_map,
    eventql::ReplicationScheme* replication_scheme,
    eventql::CompactionWorker* cstable_index,
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
  cdb_->setNamespaceConfigChangeCallback(
      std::bind(
          &AnalyticsApp::configureCustomer,
          this,
          std::placeholders::_1));

  cdb_->listNamespaces([this] (const NamespaceConfig& cfg) {
    configureCustomer(cfg);
  });

  cdb_->setTableConfigChangeCallback(
      std::bind(
          &AnalyticsApp::configureTable,
          this,
          std::placeholders::_1));

  cdb_->listTables([this] (const TableDefinition& tbl) {
    configureTable(tbl);
  });
}

RefPtr<csql::TableProvider> AnalyticsApp::getTableProvider(
    const String& customer) const {
  return eventql::SQLEngine::tableProviderForNamespace(
        partition_map_,
        replication_scheme_,
        auth_,
        customer);
}

eventql::TSDBService* AnalyticsApp::getTSDBNode() const {
  return tsdb_node_;
}

void AnalyticsApp::createTable(const TableDefinition& tbl) {
  cdb_->updateTableConfig(tbl);
}

void AnalyticsApp::updateTable(const TableDefinition& tbl, bool force) {
  cdb_->updateTableConfig(tbl, force);
}

void AnalyticsApp::configureTable(const TableDefinition& tbl) {
  tsdb_node_->createTable(tbl);
}

void AnalyticsApp::configureCustomer(const NamespaceConfig& config) {
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

} // namespace eventql
