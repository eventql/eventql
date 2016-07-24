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
#include <eventql/cli/commands/cluster_remove_server.h>
#include <eventql/util/cli/flagparser.h>
#include "eventql/config/config_directory.h"
#include "eventql/util/random.h"

namespace eventql {
namespace cli {

const String ClusterRemoveServer::kName_ = "cluster-remove-server";
const String ClusterRemoveServer::kDescription_ =
    "Remove an existing server from an existing cluster.";

ClusterRemoveServer::ClusterRemoveServer(
    RefPtr<ProcessConfig> process_cfg) :
    process_cfg_(process_cfg) {}

Status ClusterRemoveServer::execute(
    const std::vector<std::string>& argv,
    FileInputStream* stdin_is,
    OutputStream* stdout_os,
    OutputStream* stderr_os) {
  ::cli::FlagParser flags;

  flags.defineFlag(
      "server_name",
      ::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "node name",
      "<string>");

  flags.defineFlag(
      "soft",
      ::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "switch",
      "<switch>");

  flags.defineFlag(
      "hard",
      ::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "switch",
      "<string>");

  try {
    flags.parseArgv(argv);

    bool remove_hard = flags.isSet("hard");
    bool remove_soft = flags.isSet("soft");
    if (!(remove_hard ^ remove_soft)) {
      stderr_os->write("ERROR: either --hard or --soft must be set\n");
      return Status(eFlagError);
    }

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

    auto cfg = cdir->getServerConfig(flags.getString("server_name"));
    if (remove_soft) {
      cfg.set_is_leaving(true);
    }
    if (remove_hard) {
      cfg.set_is_dead(true);
    }

    cdir->updateServerConfig(cfg);
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

const String& ClusterRemoveServer::getName() const {
  return kName_;
}

const String& ClusterRemoveServer::getDescription() const {
  return kDescription_;
}

void ClusterRemoveServer::printHelp(OutputStream* stdout_os) const {
  stdout_os->write(StringUtil::format(
      "\nevqlctl-$0 - $1\n\n", kName_, kDescription_));

  stdout_os->write(
      "Usage: evqlctl cluster-remove-server [OPTIONS]\n"
      "  --server_name            The name of the server to add.\n"
      "  --soft                   Enable the soft-leave operation.\n"
      "  --hard                   Enable the hard-leave operation.\n");
}

} // namespace cli
} // namespace eventql


