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
#pragma once
#include "eventql/eventql.h"
#include "eventql/config/config_directory.h"

namespace eventql {

class LegacyConfigDirectory : public ConfigDirectory {
public:

  LegacyConfigDirectory(
      const String& path,
      InetAddr master_addr);

  ClusterConfig getClusterConfig() const override;

  void updateClusterConfig(ClusterConfig config) override;

  void setClusterConfigChangeCallback(
      Function<void (const ClusterConfig& cfg)> fn) override;

  RefPtr<NamespaceConfigRef> getNamespaceConfig(
      const String& customer_key) const override;

  void updateNamespaceConfig(NamespaceConfig config) override;

  void listNamespaces(
      Function<void (const NamespaceConfig& cfg)> fn) const override;

  void setNamespaceConfigChangeCallback(
      Function<void (const NamespaceConfig& cfg)> fn) override;

  TableDefinition getTableConfig(
      const String& db_namespace,
      const String& table_name) const override;

  void updateTableConfig(
      const TableDefinition& table,
      bool force = false) override;

  void listTables(
      Function<void (const TableDefinition& tbl)> fn) const override;

  void setTableConfigChangeCallback(
      Function<void (const TableDefinition& tbl)> fn) override;

  bool start() override;
  void stop() override;

protected:

  enum ConfigTopic : uint64_t {
    CUSTOMERS = 1,
    TABLES = 2,
    USERDB = 3,
    CLUSTERCONFIG = 4
  };

  class ConfigDirectoryClient {
  public:

    ConfigDirectoryClient(InetAddr master_addr);

    ClusterConfig fetchClusterConfig();
    ClusterConfig updateClusterConfig(const ClusterConfig& config);

  protected:
    InetAddr master_addr_;
  };

  void sync();

  void loadNamespaceConfigs();
  HashMap<String, uint64_t> fetchMasterHeads() const;

  void syncObject(const String& obj);

  void syncClusterConfig();
  void commitClusterConfig(const ClusterConfig& config);

  void syncNamespaceConfig(const String& customer);
  void commitNamespaceConfig(const NamespaceConfig& config);

  void syncTableDefinitions(const String& customer);
  void commitTableDefinition(const TableDefinition& tbl);

  InetAddr master_addr_;
  ConfigDirectoryClient cclient_;
  uint64_t topics_;
  RefPtr<mdb::MDB> db_;
  mutable std::mutex mutex_;
  ClusterConfig cluster_config_;
  HashMap<String, RefPtr<NamespaceConfigRef>> customers_;

  Vector<Function<void (const ClusterConfig& cfg)>> on_cluster_change_;
  Vector<Function<void (const NamespaceConfig& cfg)>> on_customer_change_;
  Vector<Function<void (const TableDefinition& cfg)>> on_table_change_;

  std::atomic<bool> watcher_running_;
  std::thread watcher_thread_;
};

} // namespace eventql
