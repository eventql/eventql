/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/SHA1.h>
#include <eventql/CustomerConfig.h>
#include <eventql/core/ClusterConfig.pb.h>
#include <eventql/TableDefinition.h>

using namespace stx;

namespace zbase {

class ConfigDirectoryMaster {
public:

  ConfigDirectoryMaster(const String& path);

  ClusterConfig fetchClusterConfig() const;
  ClusterConfig updateClusterConfig(ClusterConfig config);

  CustomerConfig fetchCustomerConfig(const String& customer_key) const;
  CustomerConfig updateCustomerConfig(CustomerConfig config);

  TableDefinition fetchTableDefinition(
      const String& customer_key,
      const String& table_key);
  TableDefinitionList fetchTableDefinitions(const String& customer_key);
  TableDefinition updateTableDefinition(
      TableDefinition table,
      bool force = false);

  UserDB fetchUserDB();
  UserConfig fetchUserConfig(
      const String& userid);
  UserConfig updateUserConfig(
      UserConfig table,
      bool force = false);

  Vector<Pair<String, uint64_t>> heads() const;

protected:

  void loadHeads();

  mutable std::mutex mutex_;
  String customerdb_path_;
  String userdb_path_;
  String clusterdb_path_;
  HashMap<String, uint64_t> heads_;
};

} // namespace zbase
