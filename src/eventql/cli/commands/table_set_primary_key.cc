/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include <eventql/cli/commands/table_set_primary_key.h>
#include "eventql/util/random.h"
#include <eventql/util/cli/flagparser.h>
#include "eventql/util/logging.h"
#include "eventql/config/config_directory.h"
#include "eventql/db/metadata_operation.h"
#include "eventql/db/metadata_coordinator.h"
#include "eventql/db/server_allocator.h"

namespace eventql {
namespace cli {

const String TableSetPrimaryKey::kName_ = "table-set-primary-key";
const String TableSetPrimaryKey::kDescription_ = "Set primary key";

TableSetPrimaryKey::TableSetPrimaryKey(
    RefPtr<ProcessConfig> process_cfg) :
    process_cfg_(process_cfg) {}

Status TableSetPrimaryKey::execute(
    const std::vector<std::string>& argv,
    FileInputStream* stdin_is,
    OutputStream* stdout_os,
    OutputStream* stderr_os) {
  ::cli::FlagParser flags;
  flags.defineFlag(
      "cluster_name",
      ::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "node name",
      "<string>");

  flags.defineFlag(
      "namespace",
      ::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "namespace",
      "<string>");

  flags.defineFlag(
      "table_name",
      ::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "table name",
      "<string>");

  flags.defineFlag(
      "primary_key",
      ::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "key",
      "<string>");

  try {
    flags.parseArgv(argv);
    auto pkey = StringUtil::split(flags.getString("primary_key"), ",");

    ScopedPtr<ConfigDirectory> cdir;
    {
      auto rc = ConfigDirectoryFactory::getConfigDirectoryForClient(
          process_cfg_.get(),
          &cdir);

      if (rc.isSuccess()) {
        rc = cdir->start();
      }

      if (!rc.isSuccess()) {
        stderr_os->write(StringUtil::format("ERROR: $0\n", rc.message()));
        return rc;
      }
    }

    auto cfg = cdir->getTableConfig(
        flags.getString("namespace"),
        flags.getString("table_name"));

    cfg.mutable_config()->clear_primary_key();
    for (const auto& k : pkey) {
      cfg.mutable_config()->add_primary_key(k);
    }

    auto replication_factor = cdir.get()->getClusterConfig().replication_factor();
    // generate new metadata file
    Set<String> servers;
    ServerAllocator server_alloc(cdir.get());
    {
      auto rc = server_alloc.allocateServers(replication_factor, &servers);
      if (!rc.isSuccess()) {
        return rc;
      }
    }

    MetadataFile::PartitionMapEntry initial_partition;
    initial_partition.begin = "";
    initial_partition.partition_id = Random::singleton()->sha1();
    initial_partition.splitting = false;
    for (const auto& s : servers) {
      MetadataFile::PartitionPlacement p;
      p.server_id = s;
      p.placement_id = Random::singleton()->random64();
      initial_partition.servers.emplace_back(p);
    }

    auto txnid = Random::singleton()->sha1();
    MetadataFile metadata_file(txnid, 1, KEYSPACE_UINT64, { initial_partition });

    eventql::MetadataCoordinator coordinator(cdir.get());
    auto rc = coordinator.createFile(
        flags.getString("namespace"),
        flags.getString("table_name"),
        metadata_file,
        Vector<String>(servers.begin(), servers.end()));

    if (!rc.isSuccess()) {
      return rc;
    }

    cfg.mutable_config()->set_partitioner(TBL_PARTITION_UINT64);
    cfg.mutable_config()->set_storage(eventql::TBL_STORAGE_COLSM);
    cfg.set_metadata_txnid(txnid.data(), txnid.size());
    cfg.set_metadata_txnseq(1);
    for (const auto& s : servers) {
      cfg.add_metadata_servers(s);
    }

    iputs("cfg: $0", cfg.DebugString());
    cdir->updateTableConfig(cfg);
    cdir->stop();
  } catch (const Exception& e) {
    stderr_os->write(StringUtil::format(
        "$0: $1\n",
        e.getTypeName(),
        e.getMessage()));
    return Status(e);
  }

  return Status::success();
}

const String& TableSetPrimaryKey::getName() const {
  return kName_;
}

const String& TableSetPrimaryKey::getDescription() const {
  return kDescription_;
}

void TableSetPrimaryKey::printHelp(OutputStream* stdout_os) const {
  stdout_os->write(StringUtil::format(
      "\nevqlctl-$0 - $1\n\n", kName_, kDescription_));

  stdout_os->write(
      "Usage: evqlctl table-set-primary-key [OPTIONS]\n"
      "  --namespace              The name of the namespace.\n"
      "  --cluster_name           The name of the cluster.\n"
      "  --table_name             The name of the table to split.\n"
      "  --primary_key            The primary key (comma separated)\n");
}

} // namespace cli
} // namespace eventql

