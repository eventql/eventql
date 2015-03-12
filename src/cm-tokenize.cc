/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "fnord-base/io/fileutil.h"
#include "fnord-base/application.h"
#include "fnord-base/logging.h"
#include "fnord-base/cli/flagparser.h"
#include "common.h"

using namespace fnord;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  //flags.defineFlag(
  //    "lang",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "language",
  //    "<lang>");
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

  for (String line; std::getline(std::cin, line); ) {
    Set<String> tokens;
    cm::tokenizeAndStem(cm::Language::GERMAN, line, &tokens);
    fnord::iputs("\ninput: $0\noutput: $1", line, tokens);
  }

  return 0;
}

