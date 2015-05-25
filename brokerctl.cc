/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <unistd.h>
#include "fnord-base/stdtypes.h"
#include "fnord-base/inspect.h"
#include "fnord-base/application.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-base/cli/CLI.h"
#include "fnord-base/thread/eventloop.h"
#include "fnord-http/httpconnectionpool.h"
#include "fnord-feeds/BrokerClient.h"

using namespace fnord;

void cmd_monitor(const cli::FlagParser& flags) {
  fnord::iputs("monitor", 1);

}

void cmd_export(const cli::FlagParser& flags) {
  fnord::iputs("export", 1);

  fnord::thread::EventLoop ev;

  auto evloop_thread = std::thread([&ev] {
    ev.run();
  });

  http::HTTPConnectionPool http(&ev);
  feeds::BrokerClient broker(&http);

  feeds::TopicCursor cursor;
  cursor.set_topic(flags.getString("topic"));

  Duration poll_interval(0.5 * kMicrosPerSecond);

  Vector<InetAddr> servers;
  servers.emplace_back(InetAddr::resolve("nue03.prod.fnrd.net:7001"));
  servers.emplace_back(InetAddr::resolve("nue02.prod.fnrd.net:7001"));

  for (;;) {
    size_t n = 0;

    for (const auto& server : servers) {
      auto msgs = broker.fetchNext(server, &cursor, 100);
      n += msgs.messages().size();
      fnord::iputs("res: $0, $1", msgs.messages().size(), cursor.DebugString());
    }

    if (n == 0) {
      usleep(poll_interval.microseconds());
    }
  }

  evloop_thread.join();
}

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "loglevel",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  cli::CLI cli;

  /* command: monitor */
  auto monitor_cmd = cli.defineCommand("monitor");
  monitor_cmd->onCall(std::bind(&cmd_monitor, std::placeholders::_1));

  /* command: export */
  auto export_cmd = cli.defineCommand("export");
  export_cmd->onCall(std::bind(&cmd_export, std::placeholders::_1));

  export_cmd->flags().defineFlag(
      "topic",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "topic",
      "<topic>");

  export_cmd->flags().defineFlag(
      "output_path",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "output file path",
      "<path>");

  export_cmd->flags().defineFlag(
      "output_prefix",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "output filename prefix",
      "<prefix>");


  cli.call(flags.getArgv());
  return 0;
}

