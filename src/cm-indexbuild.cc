/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#define BOOST_NO_CXX11_NUMERIC_LIMITS 1
#include <stdlib.h>
#include <unistd.h>
#include "fnord/base/application.h"
#include "fnord/base/logging.h"
#include "fnord/base/thread/eventloop.h"
#include "fnord/base/thread/threadpool.h"
#include "fnord/cli/flagparser.h"
#include "fnord/net/http/httprouter.h"
#include "fnord/net/http/httpserver.h"
#include "fnord-fts/fts.h"
#include "fnord-fts/fts_common.h"
#include "customernamespace.h"

using namespace fnord;

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr();

  cli::FlagParser flags;

  flags.defineFlag(
      "cmenv",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      "dev",
      "cm env",
      "<env>");

  flags.defineFlag(
      "cmdata",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "clickmatcher app data dir",
      "<path>");

  flags.defineFlag(
      "cmcustomer",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "clickmatcher customer",
      "<key>");

  flags.parseArgv(argc, argv);

  logInfo("cm.indexbuild", "Starting cm-indexbuild");

  /* set up cmdata */
  auto cmdata_path = flags.getString("cmdata");
  if (!FileUtil::isDirectory(cmdata_path)) {
    RAISEF(kIOError, "no such directory: $0", cmdata_path);
  }

  auto cmcustomer = flags.getString("cmcustomer");

  auto index_path = FileUtil::joinPaths(
      cmdata_path,
      StringUtil::format("index/$0/fts", cmcustomer));
  FileUtil::mkdir_p(index_path);

  auto index_writer =
      fts::newLucene<fts::IndexWriter>(
          fts::FSDirectory::open(StringUtil::convertUTF8To16(index_path)),
          fts::newLucene<fts::StandardAnalyzer>(
              fts::LuceneVersion::LUCENE_CURRENT),
          true,
          fts::IndexWriter::MaxFieldLengthLIMITED);

  auto doc = fts::newLucene<fts::Document>();
  doc->add(fts::newLucene<fts::Field>(L"title", L"my fnordy document"));
  index_writer->addDocument(doc);

  index_writer->commit();
  index_writer->close();

  return 0;
}

