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
#include <common/CustomerConfig.h>
#include <common/TableDefinition.h>

using namespace stx;

namespace cm {

class ConfigDirectoryMaster {
public:

  ConfigDirectoryMaster(const String& path);

  CustomerConfig fetchCustomerConfig(const String& customer_key) const;
  CustomerConfig updateCustomerConfig(CustomerConfig config);

  TableDefinition fetchTableDefinition(
      const String& customer_key,
      const String& table_key);
  TableDefinitionList fetchTableDefinitions(const String& customer_key);
  TableDefinition updateTableDefinition(
      TableDefinition table,
      bool force = false);

  Vector<Pair<String, uint64_t>> heads() const;

protected:

  void loadHeads();

  mutable std::mutex mutex_;
  String db_path_;
  HashMap<String, uint64_t> heads_;
};

} // namespace cm
