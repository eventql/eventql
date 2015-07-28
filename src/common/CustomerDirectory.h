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

  RefPtr<CustomerConfigRef> configFor(const String& customer_key);
  void updateCustomerConfig(CustomerConfig config);

  void addTableDefinition(const TableDefinition& table) const;
  void updateTableDefinition(const TableDefinition& table) const;
  void listTableDefinitions(
      Function<void (const TableDefinition& table)> fn) const;

protected:
  RefPtr<mdb::MDB> db_;
  std::mutex mutex_;
  HashMap<String, RefPtr<CustomerConfigRef>> customers_;
};

} // namespace cm
