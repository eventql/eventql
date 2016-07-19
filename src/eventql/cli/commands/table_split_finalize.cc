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
#include <eventql/cli/commands/table_split_finalize.h>
#include "eventql/util/random.h"
#include <eventql/util/cli/flagparser.h>
#include "eventql/util/logging.h"
#include "eventql/config/config_directory.h"
#include "eventql/db/metadata_operation.h"
#include "eventql/db/metadata_coordinator.h"
#include "eventql/db/server_allocator.h"

namespace eventql {
namespace cli {

const String TableSplitFinalize::kName_ = "table-split-finalize";
const String TableSplitFinalize::kDescription_ = "Split partition (finalize split)";

TableSplitFinalize::TableSplitFinalize(
    RefPtr<ProcessConfig> process_cfg) :
    process_cfg_(process_cfg) {}

Status TableSplitFinalize::execute(
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
      "partition_id",
      ::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "table name",
      "<string>");

  try {
    flags.parseArgv(argv);

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

    auto table_cfg = cdir->getTableConfig(
        flags.getString("namespace"),
        flags.getString("table_name"));

    auto partition_id = SHA1Hash::fromHexString(flags.getString("partition_id"));
    FinalizeSplitOperation op;
    op.set_partition_id(partition_id.data(), partition_id.size());

    MetadataOperation envelope(
        flags.getString("namespace"),
        flags.getString("table_name"),
        METAOP_FINALIZE_SPLIT,
        SHA1Hash(
            table_cfg.metadata_txnid().data(),
            table_cfg.metadata_txnid().size()),
        Random::singleton()->sha1(),
        *msg::encode(op));

    MetadataCoordinator coordinator(cdir.get());
    {
      auto rc = coordinator.performAndCommitOperation(
          flags.getString("namespace"),
          flags.getString("table_name"),
          envelope);

      if (!rc.isSuccess()) {
        stderr_os->write(StringUtil::format("ERROR: $0\n", rc.message()));
        return rc;
      } else {
        stdout_os->write("SUCCESS\n");
      }
    }

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

const String& TableSplitFinalize::getName() const {
  return kName_;
}

const String& TableSplitFinalize::getDescription() const {
  return kDescription_;
}

void TableSplitFinalize::printHelp(OutputStream* stdout_os) const {
  stdout_os->write(StringUtil::format(
      "\nevqlctl-$0 - $1\n\n", kName_, kDescription_));

  stdout_os->write(
      "Usage: evqlctl table-split [OPTIONS]\n"
      "  --namespace              The name of the namespace.\n"
      "  --cluster_name           The name of the cluster.\n"
      "  --table_name             The name of the table to split.\n"
      "  --partition_id           The id of the partition to split.\n"
      "  --split_point            \n");
}

} // namespace cli
} // namespace eventql




