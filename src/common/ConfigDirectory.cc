/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <unistd.h>
#include <stx/exception.h>
#include <stx/uri.h>
#include <stx/protobuf/msg.h>
#include <stx/csv/CSVInputStream.h>
#include <common/ConfigDirectory.h>

using namespace stx;

namespace cm {

ConfigDirectory::ConfigDirectory(
    const String& path,
    const InetAddr master_addr) :
    master_addr_(master_addr),
    watcher_running_(false) {
  mdb::MDBOptions mdb_opts;
  mdb_opts.data_filename = "cdb.db",
  mdb_opts.lock_filename = "cdb.db.lck";
  mdb_opts.duplicate_keys = false;

  db_ = mdb::MDB::open(path, mdb_opts);

  listCustomers([this] (const CustomerConfig& cfg) {
    customers_.emplace(cfg.customer(), new CustomerConfigRef(cfg));
  });

  sync();
}

RefPtr<CustomerConfigRef> ConfigDirectory::configFor(
    const String& customer_key) const {
  std::unique_lock<std::mutex> lk(mutex_);

  auto iter = customers_.find(customer_key);
  if (iter == customers_.end()) {
    RAISEF(kNotFoundError, "customer not found: $0", customer_key);
  }

  return iter->second;
}

void ConfigDirectory::listCustomers(
    Function<void (const CustomerConfig& cfg)> fn) const {
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

    fn(msg::decode<CustomerConfig>(value));
  } while (cursor->getNext(&key, &value));

  cursor->close();
}

void ConfigDirectory::onCustomerConfigChange(
    Function<void (const CustomerConfig& cfg)> fn) {
  std::unique_lock<std::mutex> lk(mutex_);
  on_customer_change_.emplace_back(fn);
}

void ConfigDirectory::addTableDefinition(const TableDefinition& table) {
  std::unique_lock<std::mutex> lk(mutex_);

  if (!StringUtil::isShellSafe(table.table_name())) {
    RAISEF(
        kIllegalArgumentError,
        "invalid table name: '$0'",
        table.table_name());
  }

  if (!table.has_schema_name() && !table.has_schema_inline()) {
    RAISEF(
        kIllegalArgumentError,
        "can't add table without a schema: '$0'",
        table.table_name());
  }

  auto db_key = StringUtil::format(
      "tbl~$0~$1",
      table.customer(),
      table.table_name());

  auto buf = msg::encode(table);

  auto txn = db_->startTransaction(false);
  txn->autoAbort();

  txn->insert(db_key.data(), db_key.size(), buf->data(), buf->size());
  txn->commit();

  for (const auto& cb : on_table_change_) {
    cb(table);
  }
}

void ConfigDirectory::updateTableDefinition(const TableDefinition& table) {
  std::unique_lock<std::mutex> lk(mutex_);

  if (!StringUtil::isShellSafe(table.table_name())) {
    RAISEF(
        kIllegalArgumentError,
        "invalid table name: '$0'",
        table.table_name());
  }

  if (!table.has_schema_name() && !table.has_schema_inline()) {
    RAISEF(
        kIllegalArgumentError,
        "can't add table without a schema: '$0'",
        table.table_name());
  }

  auto db_key = StringUtil::format(
      "tbl~$0~$1",
      table.customer(),
      table.table_name());

  auto buf = msg::encode(table);

  auto txn = db_->startTransaction(false);
  txn->autoAbort();

  txn->update(db_key.data(), db_key.size(), buf->data(), buf->size());
  txn->commit();

  for (const auto& cb : on_table_change_) {
    cb(table);
  }
}

void ConfigDirectory::listTableDefinitions(
    Function<void (const TableDefinition& table)> fn) const {
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

void ConfigDirectory::onTableDefinitionChange(
    Function<void (const TableDefinition& tbl)> fn) {
  std::unique_lock<std::mutex> lk(mutex_);
  on_table_change_.emplace_back(fn);
}

void ConfigDirectory::sync() {
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

void ConfigDirectory::syncObject(const String& obj) {
  logDebug("analyticsd", "Syncing config object '$0' from master", obj);

  static const String kCustomerPrefix = "customers/";
  if (StringUtil::beginsWith(obj, kCustomerPrefix)) {
    syncCustomerConfig(obj.substr(kCustomerPrefix.size()));
  }
}

void ConfigDirectory::syncCustomerConfig(const String& customer) {
  auto uri = URI(
      StringUtil::format(
          "http://$0/analytics/master/fetch_customer_config?customer=$1",
          master_addr_.hostAndPort(),
          URI::urlEncode(customer)));

  http::HTTPClient http;
  auto res = http.executeRequest(http::HTTPRequest::mkGet(uri));
  if (res.statusCode() != 200) {
    RAISEF(kRuntimeError, "error: $0", res.body().toString());
  }

  commitCustomerConfig(msg::decode<CustomerConfig>(res.body()));
}

void ConfigDirectory::commitCustomerConfig(const CustomerConfig& config) {
  auto db_key = StringUtil::format("cfg~$0", config.customer());
  auto hkey = StringUtil::format("head~customers/$0", config.customer());
  auto buf = msg::encode(config);
  auto vstr = StringUtil::toString(config.version());

  std::unique_lock<std::mutex> lk(mutex_);
  auto txn = db_->startTransaction(false);

  auto last_version = 0;
  auto last_version_str = txn->get(hkey);
  if (!last_version_str.isEmpty()) {
    last_version = std::stoull(last_version_str.get().toString());
  }

  if (last_version >= config.version()) {
    RAISE(kRuntimeError, "refusing to commit outdated version");
  }

  txn->autoAbort();
  txn->update(db_key.data(), db_key.size(), buf->data(), buf->size());
  txn->update(hkey.data(), hkey.size(), vstr.data(), vstr.size());
  txn->commit();

  customers_.emplace(config.customer(), new CustomerConfigRef(config));

  for (const auto& cb : on_customer_change_) {
    cb(config);
  }
}

HashMap<String, uint64_t> ConfigDirectory::fetchMasterHeads() const {
  auto uri = URI(
      StringUtil::format(
          "http://$0/analytics/master/heads",
          master_addr_.hostAndPort()));

  http::HTTPClient http;
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

void ConfigDirectory::startWatcher() {
  watcher_running_ = true;

  watcher_thread_ = std::thread([this] {
    while (watcher_running_.load()) {
      try {
        sync();
      } catch (const StandardException& e) {
        logCritical("analyticsd", e, "error during master sync");
      }

      usleep(500000);
    }
  });
}

void ConfigDirectory::stopWatcher() {
  if (!watcher_running_) {
    return;
  }

  watcher_running_ = false;
  watcher_thread_.join();
}

} // namespace cm
