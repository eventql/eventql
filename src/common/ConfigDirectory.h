/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/SHA1.h>
#include <stx/mdb/MDB.h>
#include <stx/net/inetaddr.h>
#include <stx/http/httpclient.h>
#include <common/CustomerConfig.h>
#include <common/TableDefinition.h>

using namespace stx;

namespace cm {

enum ConfigTopic : uint64_t {
  CUSTOMERS = 1,
  TABLES = 2
};

class ConfigDirectory {
public:

  ConfigDirectory(
      const String& path,
      InetAddr master_addr,
      uint64_t topics);

  RefPtr<CustomerConfigRef> configFor(const String& customer_key) const;
  void updateCustomerConfig(CustomerConfig config);
  void listCustomers(
      Function<void (const CustomerConfig& cfg)> fn) const;
  void onCustomerConfigChange(Function<void (const CustomerConfig& cfg)> fn);

  void updateTableDefinition(const TableDefinition& table, bool force = false);
  void listTableDefinitions(
      Function<void (const TableDefinition& tbl)> fn) const;
  void onTableDefinitionChange(Function<void (const TableDefinition& tbl)> fn);

  void sync();

  void startWatcher();
  void stopWatcher();

protected:

  void loadCustomerConfigs();
  HashMap<String, uint64_t> fetchMasterHeads() const;

  void syncObject(const String& obj);
  void syncCustomerConfig(const String& customer);
  void commitCustomerConfig(const CustomerConfig& config);

  void syncTableDefinitions(const String& customer);
  void commitTableDefinition(const TableDefinition& tbl);

  InetAddr master_addr_;
  uint64_t topics_;
  RefPtr<mdb::MDB> db_;
  mutable std::mutex mutex_;
  HashMap<String, RefPtr<CustomerConfigRef>> customers_;

  Vector<Function<void (const CustomerConfig& cfg)>> on_customer_change_;
  Vector<Function<void (const TableDefinition& cfg)>> on_table_change_;

  std::atomic<bool> watcher_running_;
  std::thread watcher_thread_;
};

} // namespace cm
