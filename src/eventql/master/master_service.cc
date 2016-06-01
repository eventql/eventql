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
#include <eventql/master/master_service.h>
#include <eventql/util/random.h>

namespace eventql {

MasterService::MasterService(
    ConfigDirectory* cdir) :
    cdir_(cdir),
    replication_factor_(3),
    metadata_replication_factor_(3) {}

Status MasterService::runOnce() {
  logInfo("evqld", "Rebalancing cluster...");

  all_servers_.clear();
  live_servers_.clear();
  for (const auto& s : cdir_->listServers()) {
    all_servers_.emplace(s.server_id());
    if (s.server_status() == SERVER_UP) {
      live_servers_.emplace_back(s.server_id());
    }
  }

  if (live_servers_.empty() || all_servers_.empty()) {
    return Status(eIllegalStateError, "cluster has no live servers");
  }

  Vector<TableDefinition> tables;
  cdir_->listTables([&tables] (const TableDefinition& tbl_cfg) {
    tables.emplace_back(tbl_cfg);
  });

  for (const auto& tbl_cfg : tables) {
    auto rc = rebalanceTable(tbl_cfg);
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  return Status::success();
}

Status MasterService::rebalanceTable(TableDefinition tbl_cfg) {
  logInfo(
      "evqld",
      "Rebalancing table '$0/$1'",
      tbl_cfg.customer(),
      tbl_cfg.table_name());

  bool tbl_cfg_dirty = false;

  // remove dead metadata servers
  {
    bool has_dead_server = false;
    Set<String> server_list;
    for (const auto& s : tbl_cfg.metadata_servers()) {
      if (all_servers_.count(s) > 0) {
        server_list.emplace(s);
      } else {
        has_dead_server = true;

        logInfo(
            "evqld",
            "Removing dead metadata server '$0' from table '$1/$2'",
            s,
            tbl_cfg.customer(),
            tbl_cfg.table_name());
      }
    }

    if (server_list.empty()) {
      logCritical(
          "evqld",
          "All metadata servers for table '$0/$1' are dead",
          tbl_cfg.customer(),
          tbl_cfg.table_name());

      has_dead_server = false;
    }

    if (has_dead_server) {
      tbl_cfg.mutable_metadata_servers()->Clear();
      for (const auto& s : server_list) {
        tbl_cfg.add_metadata_servers(s);
      }
      tbl_cfg_dirty = true;
    }
  }

  // remove leaving metadata servers if it doesnt violate our replication
  // factor constraint
  {
    // ...
  }

  // make sure we have enough metadata servers
  {
    size_t nservers = 0;
    for (const auto& s : tbl_cfg.metadata_servers()) {
      if (leaving_servers_.count(s) == 0) {
        ++nservers;
      }
    }

    for (; nservers < metadata_replication_factor_; ++nservers) {
      auto new_server = pickMetadataServer();
      logInfo(
          "evqld",
          "Adding new metadata server '$0' to table '$1/$2'",
          new_server,
          tbl_cfg.customer(),
          tbl_cfg.table_name());

      tbl_cfg.add_metadata_servers(new_server);
      table_cfg_dirty = true;
    }
  }

  if (tbl_cfg_dirty) {
    cdir_->updateTableConfig(tbl_cfg);
  }

  return Status::success();
}

String MasterService::pickMetadataServer() const {
  return pickServer();
}

String MasterService::pickServer() const {
  uint64_t idx = Random::singleton()->random64();
  return live_servers_[idx % live_servers_.size()];
}

} // namespace eventql

