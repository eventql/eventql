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
#include "stx/io/fileutil.h"
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/cli/flagparser.h"
#include "common.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>

using namespace stx;

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "lang",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "language",
      "<lang>");

  flags.defineFlag(
      "conf",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "./conf",
      "conf directory",
      "<path>");

  flags.defineFlag(
      "loglevel",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  auto lang = languageFromString(flags.getString("lang"));
  stx::fts::Analyzer analyzer(flags.getString("conf"));

  for (String line; std::getline(std::cin, line); ) {
    Set<String> tokens;
    analyzer.extractTerms(lang, line, &tokens);
    stx::iputs("\ninput: $0\noutput: $1", line, tokens);
  }

  return 0;
}

