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
#include <eventql/cli/commands/table_config_set.h>
#include "eventql/util/random.h"
#include <eventql/util/cli/flagparser.h>
#include "eventql/util/logging.h"
#include "eventql/config/config_directory.h"
#include "eventql/db/metadata_operation.h"
#include "eventql/db/metadata_coordinator.h"
#include "eventql/db/server_allocator.h"

namespace eventql {
namespace cli {

const String TableConfigSet::kName_ = "table-config-set";
const String TableConfigSet::kDescription_ = "Set table config parameters";

TableConfigSet::TableConfigSet(
    RefPtr<ProcessConfig> process_cfg) :
    process_cfg_(process_cfg) {}

Status TableConfigSet::execute(
    const std::vector<std::string>& argv,
    FileInputStream* stdin_is,
    OutputStream* stdout_os,
    OutputStream* stderr_os) {
  ::cli::FlagParser flags;
  flags.defineFlag(
      "database",
      ::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "database",
      "<string>");

  flags.defineFlag(
      "table",
      ::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "table name",
      "<string>");

  flags.defineFlag(
      "param",
      ::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "key",
      "<string>");

  flags.defineFlag(
      "value",
      ::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "key",
      "<string>");

  try {
    flags.parseArgv(argv);
    auto param = flags.getString("param");
    auto param_value = flags.getString("value");

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
        flags.getString("database"),
        flags.getString("table"));

    auto rc = Status::success();
    if (param == "disable_replication") {
      rc = setDisableReplication(param_value, &cfg);
    } else if (param == "enable_async_split") {
      rc = setEnableAsyncSplit(param_value, &cfg);
    } else if (param == "override_partition_split_threshold") {
      rc = setOverridePartitionSplitThreshold(param_value, &cfg);
    } else {
      rc = Status(eIllegalArgumentError, "invalid param");
    }

    if (!rc.isSuccess()) {
      return rc;
    }

    iputs("cfg: $0", cfg.DebugString());
    cdir->updateTableConfig(cfg);
    cdir->stop();
  } catch (const Exception& e) {
    return Status(e);
  }

  return Status::success();
}

Status TableConfigSet::setDisableReplication(
    String value,
    TableDefinition* tbl_cfg) {
  StringUtil::toLower(&value);

  if (value == "true") {
    tbl_cfg->mutable_config()->set_disable_replication(true);
    return Status::success();
  } else if (value == "false") {
    tbl_cfg->mutable_config()->set_disable_replication(false);
    return Status::success();
  } else {
    return Status(eIllegalArgumentError, "invalid value");
  }
}

Status TableConfigSet::setEnableAsyncSplit(
    String value,
    TableDefinition* tbl_cfg) {
  StringUtil::toLower(&value);

  if (value == "true") {
    tbl_cfg->mutable_config()->set_enable_async_split(true);
    return Status::success();
  } else if (value == "false") {
    tbl_cfg->mutable_config()->set_enable_async_split(false);
    return Status::success();
  } else {
    return Status(eIllegalArgumentError, "invalid value");
  }
}

Status TableConfigSet::setOverridePartitionSplitThreshold(
    String value,
    TableDefinition* tbl_cfg) {
  tbl_cfg->mutable_config()->set_override_partition_split_threshold(
      std::stoull(value));

  return Status::success();
}

const String& TableConfigSet::getName() const {
  return kName_;
}

const String& TableConfigSet::getDescription() const {
  return kDescription_;
}

void TableConfigSet::printHelp(OutputStream* stdout_os) const {
  stdout_os->write(StringUtil::format(
      "\nevqlctl-$0 - $1\n\n", kName_, kDescription_));

  stdout_os->write(
      "Usage: evqlctl table-config-set [OPTIONS]\n"
      "  --database               The name of the database to modify.\n"
      "  --table                  The name of the table to modify.\n"
      "  --param                  The parameter to set\n"
      "  --value                  The value to set the parameter to\n");
}

} // namespace cli
} // namespace eventql

