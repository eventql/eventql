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
#include <signal.h>
#include <sys/mman.h>
#include "fnord-base/io/filerepository.h"
#include "fnord-base/io/fileutil.h"
#include "fnord-base/io/mmappedfile.h"
#include "fnord-base/application.h"
#include "fnord-base/logging.h"
#include "fnord-base/random.h"
#include "fnord-base/thread/eventloop.h"
#include "fnord-base/thread/threadpool.h"
#include "fnord-base/wallclock.h"
#include "fnord-rpc/ServerGroup.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-json/json.h"
#include "fnord-json/jsonrpc.h"
#include "fnord-http/httprouter.h"
#include "fnord-http/httpserver.h"
#include "fnord-sstable/SSTableReader.h"
#include "fnord-feeds/FeedService.h"
#include "fnord-feeds/RemoteFeedFactory.h"
#include "fnord-feeds/RemoteFeedReader.h"
#include "fnord-base/stats/statsdagent.h"
#include "fnord-base/RadixTree.h"
#include "fnord-mdb/MDB.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "FeaturePack.h"
#include "FeatureIndex.h"
#include "JoinedQuery.h"

using namespace fnord;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "input",
      fnord::cli::FlagParser::T_STRING,
      true,
      "i",
      NULL,
      "input file",
      "<filename>");

  flags.defineFlag(
      "output",
      fnord::cli::FlagParser::T_STRING,
      true,
      "o",
      NULL,
      "output file",
      "<filename>");

  flags.defineFlag(
      "featuredb_path",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "feature db path",
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

  /* set up feature schema */
  cm::FeatureSchema feature_schema;
  feature_schema.registerFeature("shop_id", 1, 1);
  feature_schema.registerFeature("category1", 2, 1);
  feature_schema.registerFeature("category2", 3, 1);
  feature_schema.registerFeature("category3", 4, 1);

  cm::FeatureIndex feature_index(&feature_schema);

  /* open featuredb */
  auto featuredb_path = flags.getString("featuredb_path");
  auto featuredb = mdb::MDB::open(featuredb_path, true);
  auto featuredb_txn = featuredb->startTransaction(true);

  HashMap<String, uint64_t> feature_counts;

  /* open output file */
  //auto outfile = File::openFile(outputfile_path, File::O_READ | File::O_WRITE);

  /* read input table */
  auto input_file = flags.getString("input_file");
  fnord::logInfo("cm.ctrstats", "Importing sstable: $0", input_file);
  sstable::SSTableReader reader(File::openFile(input_file, File::O_READ));

  if (reader.bodySize() == 0) {
    fnord::logCritical("cm.ctrstats", "unfinished sstable: $0", input_file);
    return 1;
  }

  /* clean up */
  featuredb_txn->abort();

  //for (const auto& f : feature_counts) {
  //  if (f.second < 100) continue;
  //  outfile.write(StringUtil::format("$0\n", f.first));
  //}

  exit(0);
}

