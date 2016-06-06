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
#include "eventql/eventql.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "eventql/util/application.h"
#include "eventql/util/thread/eventloop.h"
#include "eventql/util/cli/flagparser.h"
#include <eventql/config/process_config.h>
#include <eventql/cli/commands/cluster_add_server.h>
#include <eventql/cli/commands/cluster_create.h>
#include <eventql/cli/commands/cluster_status.h>
#include <eventql/cli/commands/cluster_remove_server.h>
#include <eventql/cli/commands/namespace_create.h>
#include <eventql/cli/commands/rebalance.h>
#include <eventql/cli/commands/table_split.h>

using namespace eventql;

thread::EventLoop ev;

String getCommandName(Vector<String> cmd_argv) {
  String cmd_name;
  if (cmd_argv.size() == 0) {
    return cmd_name;
  }

  cmd_name = cmd_argv[0];
  if (StringUtil::beginsWith(cmd_name, "--")) {
    cmd_name.erase(0, 2);
  } if (StringUtil::beginsWith(cmd_name, "-")) {
    cmd_name.erase(cmd_name.begin());
  }

  return cmd_name;
}

int main(int argc, const char** argv) {
  ::cli::FlagParser flags;
  flags.defineFlag(
      "loglevel",
      ::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.defineFlag(
      "c",
      ::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "path to config file",
      "<config_file>");

  flags.parseArgv(argc, argv);
  auto stdin_is = FileInputStream::getStdin();
  auto stdout_os = OutputStream::getStdout();
  auto stderr_os = OutputStream::getStderr();
  ProcessConfigBuilder config_builder;

  Application::init();
  Application::logToStderr("evqlctl");

  if (flags.isSet("c")) {
    auto rc = config_builder.loadFile(flags.getString("c"));
    if (!rc.isSuccess()) {
      stderr_os->write(StringUtil::format("ERROR: $0", rc.message()));
      return 1;
    }
  } else {
    auto rc = config_builder.loadDefaultConfigFile();
    if (!rc.isSuccess()) {
      //stderr_os->write("WARNING: could not load config file");
    }
  }

  auto process_config = config_builder.getConfig();

  List<eventql::cli::CLICommand*> commands;
  commands.emplace_back(new eventql::cli::ClusterAddServer(process_config));
  commands.emplace_back(new eventql::cli::ClusterCreate(process_config));
  commands.emplace_back(new eventql::cli::ClusterRemoveServer(process_config));
  commands.emplace_back(new eventql::cli::ClusterStatus(process_config));
  commands.emplace_back(new eventql::cli::NamespaceCreate(process_config));
  commands.emplace_back(new eventql::cli::Rebalance(process_config));
  commands.emplace_back(new eventql::cli::TableSplit(process_config));

  Vector<String> cmd_argv = flags.getArgv();
  String cmd_name = getCommandName(cmd_argv);
  if (cmd_name.empty()) {
    //if (default_cmd_.empty()) {
      stderr_os->write(
        "evqlctl: command is not specified. See 'evqlctl --help'.\n");
      return 1;
    //}

    //cmd_name = default_cmd_;

  } else {
    cmd_argv.erase(cmd_argv.begin());
  }


  if (cmd_name == "help") {
    cmd_name = getCommandName(cmd_argv);
    if (cmd_name.empty()) {
      stdout_os->write(
        "Usage: evqlctl [OPTIONS] <command> [<args>]\n\n"
        "   -?, --help                Display this help text and exit\n"
        "   -C <path>                 Set the path to the config file\n"
        "   -c name=value             Overwrite a config file value\n\n"
        "evqctl commands:\n"
      );

      for (const auto c : commands) {
        stdout_os->printf("   %-26.26s", c->getName().c_str());
        stdout_os->printf("%-86.86s\n", c->getDescription().c_str());
      }
      return 0;
    }

    for (auto c : commands) {
      if (c->getName() == cmd_name) {
        c->printHelp(stdout_os.get());
        return 0;
      }
    }

    stderr_os->write(StringUtil::format(
        "evqlctl: No manual entry for evqlctl '$0'\n",
        cmd_name));
    return 1;
  }

  for (auto c : commands) {
    if (c->getName() == cmd_name) {
      auto rc = c->execute(
          cmd_argv,
          stdin_is.get(),
          stdout_os.get(),
          stderr_os.get());
      return rc.isSuccess() ? 0 : 1;
    }
  }

  stderr_os->write(StringUtil::format(
      "evqlctl: '$0' is not a evqlctl command. See 'evqlctl --help'.\n",
      cmd_name));

  return 1;

}

