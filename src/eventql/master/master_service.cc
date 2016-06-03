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
#include <eventql/db/server_allocator.h>

namespace eventql {

MasterService::MasterService(
    ConfigDirectory* cdir) :
    cdir_(cdir),
    metadata_coordinator_(cdir),
    metadata_client_(cdir),
    replication_factor_(3),
    metadata_replication_factor_(3) {}

Status MasterService::runOnce() {
  logInfo("evqld", "Rebalancing cluster...");

  all_servers_.clear();
  for (const auto& s : cdir_->listServers()) {
    if (s.is_dead()) {
      continue;
    }

    all_servers_.emplace(s.server_id());
  }

  if (all_servers_.empty()) {
    return Status(eIllegalStateError, "cluster has no live servers");
  }

  Vector<TableDefinition> tables;
  cdir_->listTables([&tables] (const TableDefinition& tbl_cfg) {
    tables.emplace_back(tbl_cfg);
  });

  for (const auto& tbl_cfg : tables) {
    if (tbl_cfg.config().partitioner() != TBL_PARTITION_TIMEWINDOW) {
      continue;
    }

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

  // fetch latest metadata file
  MetadataFile metadata;
  auto rc = metadata_client_.fetchMetadataFile(tbl_cfg, &metadata);
  if (!rc.isSuccess()) {
    return rc;
  }

  bool tbl_cfg_dirty = false;
  bool metadata_servers_changed = false;

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
    Set<String> current_metadata_servers;
    size_t nservers = 0;
    for (const auto& s : tbl_cfg.metadata_servers()) {
      current_metadata_servers.emplace(s);

      if (all_servers_.count(s) > 0 &&
          leaving_servers_.count(s) == 0) {
        ++nservers;
      }
    }

    if (nservers < metadata_replication_factor_) {
      auto new_metadata_servers = current_metadata_servers;
      ServerAllocator server_alloc(cdir_);
      {
        auto rc = server_alloc.allocateServers(
            metadata_replication_factor_ - nservers,
            &new_metadata_servers);
        if (!rc.isSuccess()) {
          return rc;
        }
      }

      for (const auto& s : new_metadata_servers) {
        if (current_metadata_servers.count(s) == 0) {
          logInfo(
              "evqld",
              "Adding new metadata server to table '$1/$2': $0",
              s,
              tbl_cfg.customer(),
              tbl_cfg.table_name());

          tbl_cfg.add_metadata_servers(s);
        }
      }

      tbl_cfg_dirty = true;
    }
  }

  // if we made a change to the metadata servers above, we need to update the
  // table config and exit, otherwise we might get stuck in a state where
  // operations below here could never succeed (due to the operations being
  // blocked by missing metadata servers, and adding metadata servers being
  // blocked by the operations failing
  if (tbl_cfg_dirty) {
    cdir_->updateTableConfig(tbl_cfg);
    return Status::success();
  }


  // remove dead servers from the metadata file
  {
    Set<String> dead_servers;
    for (const auto& e : metadata.getPartitionMap()) {
      for (const auto& s : e.servers) {
        if (all_servers_.count(s.server_id) == 0) {
          dead_servers.emplace(s.server_id);
        }
      }
      for (const auto& s : e.servers_joining) {
        if (all_servers_.count(s.server_id) == 0) {
          dead_servers.emplace(s.server_id);
        }
      }
      for (const auto& s : e.servers_leaving) {
        if (all_servers_.count(s.server_id) == 0) {
          dead_servers.emplace(s.server_id);
        }
      }
    }

    if (!dead_servers.empty()) {
      logInfo(
          "evqld",
          "Removing dead servers from partition map for table '$0/$1': $2",
          tbl_cfg.customer(),
          tbl_cfg.table_name(),
          inspect(dead_servers));

      RemoveDeadServersOperation opdata;
      for (const auto& s : dead_servers) {
        opdata.add_server_ids(s);
      }

      tbl_cfg_dirty = true;
      auto rc = performMetadataOperation(
          &tbl_cfg,
          &metadata,
          METAOP_REMOVE_DEAD_SERVERS,
          *msg::encode(opdata));

      if (!rc.isSuccess()) {
        return rc;
      }
    }
  }

  if (tbl_cfg_dirty) {
    cdir_->updateTableConfig(tbl_cfg);
  }

  return Status::success();
}

Status MasterService::performMetadataOperation(
    TableDefinition* table_cfg,
    MetadataFile* metadata_file,
    MetadataOperationType optype,
    const Buffer& opdata) {
  auto new_txnid = Random::singleton()->sha1();

  MetadataOperation op(
      table_cfg->customer(),
      table_cfg->table_name(),
      optype,
      SHA1Hash(
          table_cfg->metadata_txnid().data(),
          table_cfg->metadata_txnid().size()),
      new_txnid,
      opdata);

  auto rc = metadata_coordinator_.performOperation(
      table_cfg->customer(),
      table_cfg->table_name(),
      op,
      Vector<String>(
          table_cfg->metadata_servers().begin(),
          table_cfg->metadata_servers().end()));

  if (!rc.isSuccess()) {
    return rc;
  }

  table_cfg->set_metadata_txnid(new_txnid.data(), new_txnid.size());
  table_cfg->set_metadata_txnseq(table_cfg->metadata_txnseq() + 1);

  return metadata_client_.fetchMetadataFile(*table_cfg, metadata_file);
}


} // namespace eventql

