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
#include <unistd.h>
#include <eventql/config/config_directory_legacy.h>
#include <eventql/server/server_stats.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/util/csv/CSVInputStream.h>

namespace eventql {

LegacyConfigDirectory::LegacyConfigDirectory(
    const String& path,
    const InetAddr master_addr) :
    master_addr_(master_addr),
    cclient_(master_addr),
    topics_(
        ConfigTopic::CUSTOMERS | ConfigTopic::TABLES | ConfigTopic::CLUSTERCONFIG),
    watcher_running_(false) {
  mdb::MDBOptions mdb_opts;
  mdb_opts.data_filename = "cdb.db",
  mdb_opts.lock_filename = "cdb.db.lck";
  mdb_opts.duplicate_keys = false;

  db_ = mdb::MDB::open(path, mdb_opts);

  if (topics_ & ConfigTopic::CUSTOMERS) {
    listNamespaces([this] (const NamespaceConfig& cfg) {
      customers_.emplace(cfg.customer(), new NamespaceConfigRef(cfg));
    });
  }

  if (topics_ & ConfigTopic::CLUSTERCONFIG) {
    auto txn = db_->startTransaction(true);
    txn->autoAbort();
    auto cc = txn->get("cluster");
    if (!cc.isEmpty()) {
      cluster_config_ = msg::decode<ClusterConfig>(cc.get());
    }
  }

  sync();
}

ClusterConfig LegacyConfigDirectory::getClusterConfig() const {
  if ((topics_ & ConfigTopic::CLUSTERCONFIG) == 0) {
    RAISE(kRuntimeError, "config topic not enabled: CLUSTERCONFIG");
  }

  std::unique_lock<std::mutex> lk(mutex_);
  return cluster_config_;
}

void LegacyConfigDirectory::updateClusterConfig(ClusterConfig config) {
  if ((topics_ & ConfigTopic::CLUSTERCONFIG) == 0) {
    RAISE(kRuntimeError, "config topic not enabled: CLUSTERCONFIG");
  }

  auto cc = cclient_.updateClusterConfig(config);
  commitClusterConfig(cc);
}

void LegacyConfigDirectory::setClusterConfigChangeCallback(
    Function<void (const ClusterConfig& cfg)> fn) {
  if ((topics_ & ConfigTopic::CLUSTERCONFIG) == 0) {
    RAISE(kRuntimeError, "config topic not enabled: CLUSTERCONFIG");
  }

  std::unique_lock<std::mutex> lk(mutex_);
  on_cluster_change_.emplace_back(fn);
}


RefPtr<NamespaceConfigRef> LegacyConfigDirectory::getNamespaceConfig(
    const String& customer_key) const {
  if ((topics_ & ConfigTopic::CUSTOMERS) == 0) {
    RAISE(kRuntimeError, "config topic not enabled: CUSTOMERS");
  }

  std::unique_lock<std::mutex> lk(mutex_);

  auto iter = customers_.find(customer_key);
  if (iter == customers_.end()) {
    RAISEF(kNotFoundError, "customer not found: $0", customer_key);
  }

  return iter->second;
}

void LegacyConfigDirectory::listNamespaces(
    Function<void (const NamespaceConfig& cfg)> fn) const {
  if ((topics_ & ConfigTopic::CUSTOMERS) == 0) {
    RAISE(kRuntimeError, "config topic not enabled: CUSTOMERS");
  }

  auto prefix = "cfg~";

  Buffer key;
  Buffer value;

  auto txn = db_->startTransaction(true);
  txn->autoAbort();

  auto cursor = txn->getCursor();
  key.append(prefix);

  if (!cursor->getFirstOrGreater(&key, &value)) {
    return;
  }

  do {
    if (!StringUtil::beginsWith(key.toString(), prefix)) {
      break;
    }

    fn(msg::decode<NamespaceConfig>(value));
  } while (cursor->getNext(&key, &value));

  cursor->close();
}

void LegacyConfigDirectory::setNamespaceConfigChangeCallback(
    Function<void (const NamespaceConfig& cfg)> fn) {
  if ((topics_ & ConfigTopic::CUSTOMERS) == 0) {
    RAISE(kRuntimeError, "config topic not enabled: CUSTOMERS");
  }

  std::unique_lock<std::mutex> lk(mutex_);
  on_customer_change_.emplace_back(fn);
}

void LegacyConfigDirectory::updateNamespaceConfig(NamespaceConfig cfg) {
  if ((topics_ & ConfigTopic::CUSTOMERS) == 0) {
    RAISE(kRuntimeError, "config topic not enabled: CUSTOMERS");
  }

  auto body = msg::encode(cfg);
  auto uri = StringUtil::format(
        "http://$0/analytics/master/update_customer_config",
        master_addr_.hostAndPort());

  http::HTTPClient http(&z1stats()->http_client_stats);
  auto res = http.executeRequest(http::HTTPRequest::mkPost(uri, *body));
  if (res.statusCode() != 201) {
    RAISEF(kRuntimeError, "error: $0", res.body().toString());
  }

  commitNamespaceConfig(msg::decode<NamespaceConfig>(res.body()));
}

TableDefinition LegacyConfigDirectory::getTableConfig(
    const String& db_namespace,
    const String& table_name) const {
  if ((topics_ & ConfigTopic::TABLES) == 0) {
    RAISE(kRuntimeError, "config topic not enabled: TABLES");
  }

  auto txn = db_->startTransaction(true);
  txn->autoAbort();

  auto db_key = StringUtil::format("tbl~$0~$1", db_namespace, table_name);
  auto value = txn->get(db_key);

  if (value.isEmpty()) {
    RAISE(kNotFoundError, "table not found");
  }

  return msg::decode<TableDefinition>(value.get());
}

void LegacyConfigDirectory::updateTableConfig(
    const TableDefinition& table,
    bool force /* = false */) {
  if ((topics_ & ConfigTopic::TABLES) == 0) {
    RAISE(kRuntimeError, "config topic not enabled: TABLES");
  }

  if (!StringUtil::isShellSafe(table.table_name())) {
    RAISEF(
        kIllegalArgumentError,
        "invalid table name: '$0'",
        table.table_name());
  }

  auto body = msg::encode(table);
  auto uri = StringUtil::format(
        "http://$0/analytics/master/update_table_definition",
        master_addr_.hostAndPort());

  if (force) {
    uri += "?force=true";
  }

  http::HTTPClient http(&z1stats()->http_client_stats);
  auto res = http.executeRequest(http::HTTPRequest::mkPost(uri, *body));
  if (res.statusCode() != 201) {
    RAISEF(kRuntimeError, "error: $0", res.body().toString());
  }

  commitTableDefinition(msg::decode<TableDefinition>(res.body()));
}

void LegacyConfigDirectory::listTables(
    Function<void (const TableDefinition& table)> fn) const {
  if ((topics_ & ConfigTopic::TABLES) == 0) {
    RAISE(kRuntimeError, "config topic not enabled: TABLES");
  }

  auto prefix = "tbl~";

  Buffer key;
  Buffer value;

  auto txn = db_->startTransaction(true);
  txn->autoAbort();

  auto cursor = txn->getCursor();
  key.append(prefix);

  if (!cursor->getFirstOrGreater(&key, &value)) {
    return;
  }

  do {
    if (!StringUtil::beginsWith(key.toString(), prefix)) {
      break;
    }

    fn(msg::decode<TableDefinition>(value));
  } while (cursor->getNext(&key, &value));

  cursor->close();
}

void LegacyConfigDirectory::setTableConfigChangeCallback(
    Function<void (const TableDefinition& tbl)> fn) {
  if ((topics_ & ConfigTopic::TABLES) == 0) {
    RAISE(kRuntimeError, "config topic not enabled: TABLES");
  }

  std::unique_lock<std::mutex> lk(mutex_);
  on_table_change_.emplace_back(fn);
}

void LegacyConfigDirectory::sync() {
  auto master_heads = fetchMasterHeads();

  Set<String> needs_update;
  {
    auto txn = db_->startTransaction(true);
    txn->autoAbort();

    for (const auto& head : master_heads) {
      auto db_key = StringUtil::format("head~$0", head.first);
      auto vstr = StringUtil::toString(head.second);

      auto lastver = txn->get(db_key);
      if (lastver.isEmpty() || lastver.get().toString() != vstr) {
        needs_update.emplace(head.first);
      }
    }

    txn->abort();
  }

  for (const auto& obj : needs_update) {
    syncObject(obj);
  }
}

void LegacyConfigDirectory::syncObject(const String& obj) {
  logDebug("eventql", "Syncing config object '$0' from master", obj);

  if (topics_ & ConfigTopic::CUSTOMERS) {
    static const String kCustomerPrefix = "customers/";
    if (StringUtil::beginsWith(obj, kCustomerPrefix)) {
      syncNamespaceConfig(obj.substr(kCustomerPrefix.size()));
      return;
    }
  }

  if (topics_ & ConfigTopic::TABLES) {
    static const String kTablesPrefix = "tables/";
    if (StringUtil::beginsWith(obj, kTablesPrefix)) {
      syncTableDefinitions(obj.substr(kTablesPrefix.size()));
      return;
    }
  }

  if ((topics_ & ConfigTopic::CLUSTERCONFIG) &&
      obj == "cluster") {
    syncClusterConfig();
    return;
  }
}

void LegacyConfigDirectory::syncClusterConfig() {
  auto cc = cclient_.fetchClusterConfig();
  commitClusterConfig(cc);
}

void LegacyConfigDirectory::commitClusterConfig(const ClusterConfig& config) {
  String db_key = "cluster";
  String hkey = "head~cluster";
  auto buf = msg::encode(config);
  auto vstr = StringUtil::toString(config.version());

  std::unique_lock<std::mutex> lk(mutex_);
  auto txn = db_->startTransaction(false);
  txn->autoAbort();

  txn->update(db_key.data(), db_key.size(), buf->data(), buf->size());
  txn->update(hkey.data(), hkey.size(), vstr.data(), vstr.size());
  txn->commit();

  cluster_config_ = config;

  for (const auto& cb : on_cluster_change_) {
    cb(config);
  }
}

void LegacyConfigDirectory::syncNamespaceConfig(const String& customer) {
  auto uri = URI(
      StringUtil::format(
          "http://$0/analytics/master/fetch_customer_config?customer=$1",
          master_addr_.hostAndPort(),
          URI::urlEncode(customer)));

  http::HTTPClient http(&z1stats()->http_client_stats);
  auto res = http.executeRequest(http::HTTPRequest::mkGet(uri));
  if (res.statusCode() != 200) {
    RAISEF(kRuntimeError, "error: $0", res.body().toString());
  }

  commitNamespaceConfig(msg::decode<NamespaceConfig>(res.body()));
}

void LegacyConfigDirectory::commitNamespaceConfig(const NamespaceConfig& config) {
  auto db_key = StringUtil::format("cfg~$0", config.customer());
  auto hkey = StringUtil::format("head~customers/$0", config.customer());
  auto buf = msg::encode(config);
  auto vstr = StringUtil::toString(config.version());

  std::unique_lock<std::mutex> lk(mutex_);
  auto txn = db_->startTransaction(false);
  txn->autoAbort();

  txn->update(db_key.data(), db_key.size(), buf->data(), buf->size());
  txn->update(hkey.data(), hkey.size(), vstr.data(), vstr.size());
  txn->commit();

  customers_[config.customer()] = new NamespaceConfigRef(config);

  for (const auto& cb : on_customer_change_) {
    cb(config);
  }
}

void LegacyConfigDirectory::syncTableDefinitions(const String& customer) {
  auto uri = URI(
      StringUtil::format(
          "http://$0/analytics/master/fetch_table_definitions?customer=$1",
          master_addr_.hostAndPort(),
          URI::urlEncode(customer)));

  http::HTTPClient http(&z1stats()->http_client_stats);
  auto res = http.executeRequest(http::HTTPRequest::mkGet(uri));
  if (res.statusCode() != 200) {
    RAISEF(kRuntimeError, "error: $0", res.body().toString());
  }

  auto tables = msg::decode<TableDefinitionList>(res.body());
  for (const auto& tbl : tables.tables()) {
    commitTableDefinition(tbl);
  }

  auto hkey = StringUtil::format("head~tables/$0", customer);
  auto vstr = StringUtil::toString(tables.version());

  std::unique_lock<std::mutex> lk(mutex_);
  auto txn = db_->startTransaction(false);
  txn->autoAbort();
  txn->update(hkey.data(), hkey.size(), vstr.data(), vstr.size());
  txn->commit();
}

void LegacyConfigDirectory::commitTableDefinition(const TableDefinition& tbl) {
  auto db_key = StringUtil::format(
      "tbl~$0~$1",
      tbl.customer(),
      tbl.table_name());

  auto buf = msg::encode(tbl);

  std::unique_lock<std::mutex> lk(mutex_);
  auto txn = db_->startTransaction(false);
  txn->autoAbort();

  auto last_version = 0;
  auto last_td = txn->get(db_key);
  if (!last_td.isEmpty()) {
    last_version = msg::decode<TableDefinition>(last_td.get()).version();
  }

  if (last_version >= tbl.version()) {
    return;
  }

  txn->update(db_key.data(), db_key.size(), buf->data(), buf->size());
  txn->commit();

  for (const auto& cb : on_table_change_) {
    cb(tbl);
  }
}

HashMap<String, uint64_t> LegacyConfigDirectory::fetchMasterHeads() const {
  auto uri = URI(
      StringUtil::format(
          "http://$0/analytics/master/heads",
          master_addr_.hostAndPort()));

  http::HTTPClient http(&z1stats()->http_client_stats);
  auto res = http.executeRequest(http::HTTPRequest::mkGet(uri));
  if (res.statusCode() != 200) {
    RAISEF(kRuntimeError, "error: $0", res.body().toString());
  }

  DefaultCSVInputStream csv(BufferInputStream::fromBuffer(&res.body()), '=');

  HashMap<String, uint64_t> heads;
  Vector<String> row;
  while (csv.readNextRow(&row)) {
    if (row.size() != 2) {
      RAISE(kRuntimeError, "invalid response");
    }

    heads[row[0]] = std::stoull(row[1]);
  }

  return heads;
}

void LegacyConfigDirectory::start() {
  watcher_running_ = true;

  watcher_thread_ = std::thread([this] {
    while (watcher_running_.load()) {
      try {
        sync();
      } catch (const StandardException& e) {
        logCritical("eventql", e, "error during master sync");
      }

      usleep(500000);
    }
  });
}

void LegacyConfigDirectory::stop() {
  if (!watcher_running_) {
    return;
  }

  watcher_running_ = false;
  watcher_thread_.join();
}

LegacyConfigDirectory::ConfigDirectoryClient::ConfigDirectoryClient(
    InetAddr master_addr) :
    master_addr_(master_addr) {}

ClusterConfig LegacyConfigDirectory::ConfigDirectoryClient::fetchClusterConfig() {
  auto uri = URI(
      StringUtil::format(
          "http://$0/analytics/master/fetch_cluster_config",
          master_addr_.hostAndPort()));

  http::HTTPClient http(&z1stats()->http_client_stats);
  auto res = http.executeRequest(http::HTTPRequest::mkGet(uri));
  if (res.statusCode() != 200) {
    RAISEF(kRuntimeError, "error: $0", res.body().toString());
  }

  return msg::decode<ClusterConfig>(res.body());
}

ClusterConfig LegacyConfigDirectory::ConfigDirectoryClient::updateClusterConfig(
    const ClusterConfig& config) {
  auto body = msg::encode(config);
  auto uri = StringUtil::format(
        "http://$0/analytics/master/update_cluster_config",
        master_addr_.hostAndPort());

  http::HTTPClient http(&z1stats()->http_client_stats);
  auto res = http.executeRequest(http::HTTPRequest::mkPost(uri, *body));
  if (res.statusCode() != 201) {
    RAISEF(kRuntimeError, "error: $0", res.body().toString());
  }

  return msg::decode<ClusterConfig>(res.body());
}

} // namespace eventql
