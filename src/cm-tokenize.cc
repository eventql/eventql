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
#include "fnord-fts/QueryAnalyzer.h"
#include "fnord-fts/GermanStemmer.h"
#include "common.h"

using namespace fnord;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "lang",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "language",
      "<lang>");

  flags.defineFlag(
      "stopwords",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "stopwords",
      "<path>");

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

  auto lang = languageFromString(flags.getString("lang"));

  fts::StopwordDictionary stopwords;
  if (flags.isSet("stopwords")) {
    stopwords.loadStopwordFile(flags.getString("stopwords"));
  } else {
    fnord::logWarning("cm-tokenize", "no stopword file provided");
  }

  fnord::fts::SynonymDictionary synonyms;

  fnord::fts::GermanStemmer german_stemmer(
      "conf/hunspell_de.aff",
      "conf/hunspell_de.dic",
      "conf/hunspell_de.hyphen",
      &synonyms);

  fts::QueryAnalyzer analyzer(&stopwords, &german_stemmer);
  for (String line; std::getline(std::cin, line); ) {
    Set<String> tokens;
    analyzer.analyze(lang, line, &tokens);
    fnord::iputs("\ninput: $0\noutput: $1", line, tokens);
  }

  return 0;
}

