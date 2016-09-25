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
#include <eventql/cli/commands/cluster_list.h>
#include <eventql/cli/commands/cluster_remove_server.h>
#include <eventql/cli/commands/database_create.h>
#include <eventql/cli/commands/table_split.h>
#include <eventql/cli/commands/table_split_finalize.h>
#include <eventql/cli/commands/table_config_set.h>

using namespace eventql;

thread::EventLoop ev;

int main(int argc, const char** argv) {
  ::cli::FlagParser flags;

  flags.defineFlag(
      "help",
      ::cli::FlagParser::T_SWITCH,
      false,
      "?",
      NULL,
      "help",
      "<help>");

  flags.defineFlag(
      "version",
      ::cli::FlagParser::T_SWITCH,
      false,
      "v",
      NULL,
      "print version",
      "<switch>");

  flags.defineFlag(
      "config",
      ::cli::FlagParser::T_STRING,
      false,
      "c",
      NULL,
      "path to config file",
      "<config_file>");

  flags.defineFlag(
      "config_set",
      ::cli::FlagParser::T_STRING,
      false,
      "C",
      NULL,
      "set config option",
      "<key>=<val>");

  flags.parseArgv(argc, argv);
  auto stdin_is = FileInputStream::getStdin();
  auto stdout_os = OutputStream::getStdout();
  auto stderr_os = OutputStream::getStderr();
  Vector<String> cmd_argv = flags.getArgv();

  Application::init();
  Application::logToStderr("evqlctl");

  /* load config */
  ProcessConfigBuilder config_builder;
  if (flags.isSet("config")) {
    auto rc = config_builder.loadFile(flags.getString("config"));
    if (!rc.isSuccess()) {
      stderr_os->write(StringUtil::format("ERROR: $0\n", rc.message()));
      return 1;
    }
  } else {
    config_builder.loadDefaultConfigFile("evqlctl");
  }

  for (const auto& opt : flags.getStrings("config_set")) {
    auto opt_key_end = opt.find("=");
    if (opt_key_end == String::npos) {
      stderr_os->write(
          StringUtil::format("ERROR: invalid config option: $0\n", opt));
      return 1;
    }

    config_builder.setProperty(
        opt.substr(0, opt_key_end),
        opt.substr(opt_key_end + 1));
  }

  auto process_config = config_builder.getConfig();

  /* init commands */
  List<eventql::cli::CLICommand*> commands;
  commands.emplace_back(new eventql::cli::ClusterAddServer(process_config));
  commands.emplace_back(new eventql::cli::ClusterCreate(process_config));
  commands.emplace_back(new eventql::cli::ClusterRemoveServer(process_config));
  commands.emplace_back(new eventql::cli::ClusterStatus(process_config));
  commands.emplace_back(new eventql::cli::ClusterList(process_config));
  commands.emplace_back(new eventql::cli::DatabaseCreate(process_config));
  commands.emplace_back(new eventql::cli::TableSplit(process_config));
  commands.emplace_back(new eventql::cli::TableSplitFinalize(process_config));
  commands.emplace_back(new eventql::cli::TableConfigSet(process_config));

  /* print help/version and exit */
  bool print_help = flags.isSet("help");
  bool print_version = flags.isSet("version");
  if (cmd_argv.size() > 0 && cmd_argv[0] == "help") {
    print_help = true;
    cmd_argv.erase(cmd_argv.begin());
  }
  auto help_topic = cmd_argv.size() > 0 ? cmd_argv.front() : "";

  if (print_version || (print_help && help_topic.empty())) {
    auto stdout_os = OutputStream::getStdout();
    stdout_os->write(
        StringUtil::format(
            "EventQL $0 ($1)\n"
            "Copyright (c) 2016, DeepCortex GmbH. All rights reserved.\n\n",
            kVersionString,
            kBuildID));
  }

  if (print_version) {
    return 0;
  }

  if (print_help) {
    if (help_topic.empty()) {
      stdout_os->write(
        "Usage: $ evqlctl [OPTIONS] <command> [<args>]\n\n"
        "   -c, --config <file>       Load config from file\n"
        "   -C name=value             Define a config value on the command line\n"
        "   -?, --help <topic>        Display a command's help text and exit\n"
        "   -v, --version             Display the version of this binary and exit\n\n"
        "evqctl commands:\n"
      );

      for (const auto c : commands) {
        stdout_os->printf("   %-26.26s", c->getName().c_str());
        stdout_os->printf("%-80.80s\n", c->getDescription().c_str());
      }

      stdout_os->write(
        "\nSee 'evqlctl help <command>' to read about a specific subcommand.\n"
      );

      return 0;
    }

    for (auto c : commands) {
      if (c->getName() == help_topic) {
        c->printHelp(stdout_os.get());
        return 0;
      }
    }

    stderr_os->write(StringUtil::format(
        "evqlctl: No manual entry for evqlctl '$0'\n",
        help_topic));
    return 1;
  }

  /* execute command */
  String cmd_name;
  if (cmd_argv.empty()) {
    stderr_os->write(
      "evqlctl: command is not specified. See 'evqlctl --help'.\n");
    return 1;
  } else {
    cmd_name = cmd_argv.front();
    cmd_argv.erase(cmd_argv.begin());
  }

  for (auto c : commands) {
    if (c->getName() == cmd_name) {
      auto rc = c->execute(
          cmd_argv,
          stdin_is.get(),
          stdout_os.get(),
          stderr_os.get());

      if (!rc.isSuccess()) {
        stderr_os->write(StringUtil::format("ERROR: $0\n", rc.message()));
      }

      return rc.isSuccess() ? 0 : 1;
    }
  }

  stderr_os->write(StringUtil::format(
      "evqlctl: '$0' is not a evqlctl command. See 'evqlctl --help'.\n",
      cmd_name));

  return 1;
}

