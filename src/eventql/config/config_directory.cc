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
#include <eventql/config/config_directory.h>
#include <eventql/config/config_directory_standalone.h>
#include <eventql/config/config_directory_zookeeper.h>

namespace eventql {

Status ConfigDirectoryFactory::getConfigDirectoryForServer(
    const ProcessConfig* cfg,
    ScopedPtr<ConfigDirectory>* cdir,
    DatabaseContext* dbctx) {
  auto config_backend = cfg->getString("cluster.coordinator");
  if (config_backend.isEmpty()) {
    return Status(
        eIllegalArgumentError,
        "missing config option: cluster.coordinator");
  }

  // zookeeper
  if (config_backend.get() == "zookeeper") {
    auto cluster_name = cfg->getString("cluster.name");
    if (cluster_name.isEmpty()) {
      return Status(
          eIllegalArgumentError,
          "missing config option: cluster.name");
    }

    auto server_name = cfg->getString("server.name");
    //if (server_name.isEmpty()) {
    //  return Status(
    //      eIllegalArgumentError,
    //      "missing config option: server.name");
    //}

    auto server_listen = cfg->getString("server.listen");
    if (server_listen.isEmpty()) {
      return Status(
          eIllegalArgumentError,
          "missing config option: server.listen");
    }

    auto zookeeper_addr = cfg->getString("cluster.zookeeper_hosts");
    if (zookeeper_addr.isEmpty()) {
      return Status(
          eIllegalArgumentError,
          "missing config option: cluster.zookeeper_hosts");
    }

    cdir->reset(
        new ZookeeperConfigDirectory(
            zookeeper_addr.get(),
            cluster_name.get(),
            server_name,
            server_listen.get()));

    return Status::success();
  }

  // standalone
  if (config_backend.get() == "standalone") {
    auto server_dir = cfg->getString("server.datadir");
    if (server_dir.isEmpty()) {
      return Status(
          eIllegalArgumentError,
          "missing config option: server.datadir");
    }

    auto server_listen = cfg->getString("server.listen");
    if (server_listen.isEmpty()) {
      return Status(
          eIllegalArgumentError,
          "missing config option: server.listen");
    }

    auto server_dir_ext = FileUtil::joinPaths(
        server_dir.get(),
        "data/standalone");

    cdir->reset(
        new StandaloneConfigDirectory(
            server_dir_ext,
            server_listen.get(),
            dbctx));

    return Status::success();
  }

  return Status(
      eNotFoundError,
      StringUtil::format(
          "invalid cluster.coordinator: $0",
          config_backend.get()));
}

Status ConfigDirectoryFactory::getConfigDirectoryForClient(
    const ProcessConfig* cfg,
    ScopedPtr<ConfigDirectory>* cdir) {
  auto config_backend = cfg->getString("cluster.coordinator");
  if (config_backend.isEmpty()) {
    return Status(
        eIllegalArgumentError,
        "missing config option: cluster.coordinator");
  }

  // zookeeper
  if (config_backend.get() == "zookeeper") {
    auto cluster_name = cfg->getString("cluster.name");
    if (cluster_name.isEmpty()) {
      return Status(
          eIllegalArgumentError,
          "missing config option: cluster.name");
    }

    auto zookeeper_addr = cfg->getString("cluster.zookeeper_hosts");
    if (zookeeper_addr.isEmpty()) {
      return Status(
          eIllegalArgumentError,
          "missing config option: cluster.zookeeper_hosts");
    }

    cdir->reset(
        new ZookeeperConfigDirectory(
            zookeeper_addr.get(),
            cluster_name.get(),
            None<String>(),
            ""));

    return Status::success();
  }

  // standalone
  if (config_backend.get() == "standalone") {
    return Status(
        eIllegalArgumentError,
        "can't connect to standalone backend");
  }

  return Status(
      eNotFoundError,
      StringUtil::format(
          "invalid cluster.coordinator: $0",
          config_backend.get()));
}

} // namespace eventql
