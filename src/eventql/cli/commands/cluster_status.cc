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
#include <eventql/cli/commands/cluster_status.h>
#include <eventql/util/cli/flagparser.h>
#include "eventql/config/config_directory_zookeeper.h"

namespace eventql {
namespace cli {

const String ClusterStatus::kName_ = "cluster-status";
const String ClusterStatus::kDescription_ = "cluster add server"; //FIXME

ClusterStatus::ClusterStatus(
    RefPtr<ProcessConfig> process_cfg) :
    process_cfg_(process_cfg) {}

Status ClusterStatus::execute(
    const std::vector<std::string>& argv,
    FileInputStream* stdin_is,
    OutputStream* stdout_os,
    OutputStream* stderr_os) {
  ::cli::FlagParser flags;
  flags.defineFlag(
      "master",
      ::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "url",
      "<addr>");

  return Status(eNotYetImplementedError, "nyi");
}

const String& ClusterStatus::getName() const {
  return kName_;
}

const String& ClusterStatus::getDescription() const {
  return kDescription_;
}

const String& ClusterStatus::printHelp() const {
  auto help_str = StringUtil::format("$0: $1", kName_, kDescription_);
  return help_str;
}

} // namespace cli
} // namespace eventql

