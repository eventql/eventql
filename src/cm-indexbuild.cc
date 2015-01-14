/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stdlib.h>
#include <unistd.h>
#include "fnord/base/application.h"
#include "fnord/base/logging.h"
#include "fnord/base/thread/eventloop.h"
#include "fnord/base/thread/threadpool.h"
#include "fnord/cli/flagparser.h"
#include "fnord/net/http/httprouter.h"
#include "fnord/net/http/httpserver.h"
#include "customernamespace.h"

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "cm_env",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      "dev",
      "cm env",
      "<env>");

  flags.defineFlag(
      "cmdata",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "clickmatcher app data dir",
      "<path>");

  flags.parseArgv(argc, argv);

  fnord::logInfo("cm.indexbuild", "Starting cm-indexbuild");

  return 0;
}

