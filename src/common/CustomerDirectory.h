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
#include <common/CustomerConfig.h>
#include <common/TableDefinition.h>

using namespace stx;

namespace cm {

class CustomerDirectory {
public:

  CustomerDirectory(const String& path);

  RefPtr<CustomerConfigRef> configFor(const String& customer_key) const;
  void updateCustomerConfig(CustomerConfig config);
  void listCustomers(
      Function<void (const CustomerConfig& cfg)> fn) const;
  void onCustomerConfigChange(Function<void (const CustomerConfig& cfg)> fn);

  void addTableDefinition(const TableDefinition& table);
  void updateTableDefinition(const TableDefinition& table);
  void listTableDefinitions(
      Function<void (const TableDefinition& tbl)> fn) const;
  void onTableDefinitionChange(Function<void (const TableDefinition& tbl)> fn);

protected:

  void loadCustomerConfigs();

  RefPtr<mdb::MDB> db_;
  mutable std::mutex mutex_;
  HashMap<String, RefPtr<CustomerConfigRef>> customers_;

  Vector<Function<void (const CustomerConfig& cfg)>> on_customer_change_;
  Vector<Function<void (const TableDefinition& cfg)>> on_table_change_;
};

} // namespace cm
