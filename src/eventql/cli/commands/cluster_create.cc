/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <eventql/cli/commands/cluster_create.h>
#include <eventql/util/cli/flagparser.h>
#include "eventql/config/config_directory.h"

namespace eventql {
namespace cli {

const String ClusterCreate::kName_ = "cluster-create";
const String ClusterCreate::kDescription_ = "Create a new cluster.";

ClusterCreate::ClusterCreate(
    RefPtr<ProcessConfig> process_cfg) :
    process_cfg_(process_cfg) {}

Status ClusterCreate::execute(
    const std::vector<std::string>& argv,
    FileInputStream* stdin_is,
    OutputStream* stdout_os,
    OutputStream* stderr_os) {
  ::cli::FlagParser flags;

  try {
    flags.parseArgv(argv);

    ScopedPtr<ConfigDirectory> cdir;
    {
      auto rc = ConfigDirectoryFactory::getConfigDirectoryForClient(
          process_cfg_.get(),
          &cdir);

      if (rc.isSuccess()) {
        rc = cdir->start(true);
      }

      if (!rc.isSuccess()) {
        stderr_os->write(StringUtil::format("ERROR: $0\n", rc.message()));
        return rc;
      }
    }

    ClusterConfig cfg;
    cfg.set_version(1);
    cdir->updateClusterConfig(cfg);

    cdir->stop();

  } catch (const Exception& e) {
    return Status(e);
  }

  stderr_os->write("Cluster successfully created\n");
  return Status::success();
}

const String& ClusterCreate::getName() const {
  return kName_;
}

const String& ClusterCreate::getDescription() const {
  return kDescription_;
}

void ClusterCreate::printHelp(OutputStream* stdout_os) const {
  stdout_os->write(StringUtil::format(
      "\nevqlctl-$0 - $1\n\n", kName_, kDescription_));

  stdout_os->write(
      "Usage: evqlctl cluster-create [OPTIONS]\n"
      "  \n"
  );
}

} // namespace cli
} // namespace eventql






