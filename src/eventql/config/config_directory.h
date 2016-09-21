/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/config/process_config.h"
#include <eventql/util/stdtypes.h>
#include <eventql/util/SHA1.h>
#include <eventql/util/net/inetaddr.h>
#include <eventql/util/http/httpclient.h>
#include <eventql/config/namespace_config.h>
#include <eventql/db/cluster_config.pb.h>
#include <eventql/db/table_config.pb.h>

namespace eventql {

enum ConfigTopic : uint64_t {
  CUSTOMERS = 1,
  TABLES = 2,
  USERDB = 3,
  CLUSTERCONFIG = 4
};

class ConfigDirectory {
public:

  virtual ~ConfigDirectory() = default;

  virtual Status start(bool create = false) = 0;
  virtual void stop() = 0;

  virtual ClusterConfig getClusterConfig() const = 0;

  virtual void updateClusterConfig(ClusterConfig config) = 0;

  virtual void setClusterConfigChangeCallback(
      Function<void (const ClusterConfig& cfg)> fn) = 0;

  virtual String getServerID() const = 0;

  virtual bool hasServerID() const { return true; }

  virtual bool electLeader() = 0;

  virtual String getLeader() const = 0;

  virtual ServerConfig getServerConfig(const String& sever_name) const = 0;

  virtual void updateServerConfig(ServerConfig config) = 0;

  virtual ReturnCode publishServerStats(ServerStats stats) = 0;

  virtual Vector<ServerConfig> listServers() const = 0;

  virtual void setServerConfigChangeCallback(
      Function<void (const ServerConfig& cfg)> fn) = 0;

  virtual RefPtr<NamespaceConfigRef> getNamespaceConfig(
      const String& customer_key) const = 0;

  virtual void updateNamespaceConfig(NamespaceConfig config) = 0;

  virtual void listNamespaces(
      Function<void (const NamespaceConfig& cfg)> fn) const = 0;

  virtual void setNamespaceConfigChangeCallback(
      Function<void (const NamespaceConfig& cfg)> fn) = 0;

  virtual TableDefinition getTableConfig(
      const String& db_namespace,
      const String& table_name) const = 0;

  virtual void updateTableConfig(
      const TableDefinition& table,
      bool force = false) = 0;

  virtual void listTables(
      Function<void (const TableDefinition& tbl)> fn) const = 0;

  virtual void setTableConfigChangeCallback(
      Function<void (const TableDefinition& tbl)> fn) = 0;

};

class ConfigDirectoryFactory {
public:

  static Status getConfigDirectoryForServer(
      const ProcessConfig* cfg,
      ScopedPtr<ConfigDirectory>* cdir);

  static Status getConfigDirectoryForClient(
      const ProcessConfig* cfg,
      ScopedPtr<ConfigDirectory>* cdir);

};

} // namespace eventql
