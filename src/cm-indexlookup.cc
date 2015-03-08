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
#include <signal.h>
#include "fnord-base/io/filerepository.h"
#include "fnord-base/io/fileutil.h"
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
#include "fnord-feeds/FeedService.h"
#include "fnord-feeds/RemoteFeedFactory.h"
#include "fnord-feeds/RemoteFeedReader.h"
#include "fnord-base/stats/statsdagent.h"
#include "fnord-fts/fts.h"
#include "fnord-fts/fts_common.h"
#include "fnord-mdb/MDB.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "FeaturePack.h"
#include "IndexRequest.h"
#include "index/IndexBuilder.h"

using namespace cm;
using namespace fnord;

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr();

  cli::FlagParser flags;

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

  flags.defineFlag(
      "docid",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "docid",
      "<docid>");

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

  /* arguments */
  auto cmcustomer = flags.getString("cmcustomer");
  DocID docid = { flags.getString("docid") };

  /* set up cmdata */
  auto cmdata_path = flags.getString("cmdata");
  if (!FileUtil::isDirectory(cmdata_path)) {
    RAISEF(kIOError, "no such directory: $0", cmdata_path);
  }

  /* set up feature schema */
  FeatureSchema feature_schema;
  feature_schema.registerFeature("shop_id", 1, 1);
  feature_schema.registerFeature("category1", 2, 1);
  feature_schema.registerFeature("category2", 3, 1);
  feature_schema.registerFeature("category3", 4, 1);
  feature_schema.registerFeature("title~de", 5, 2);

  /* open featuredb db */
  auto featuredb_path = FileUtil::joinPaths(
      cmdata_path,
      StringUtil::format("index/$0/db", cmcustomer));

  auto featuredb = mdb::MDB::open(featuredb_path, true);
  auto featuredb_txn = featuredb->startTransaction(true);

  /* get features */
  FeaturePack features;
  FeatureIndex feature_index(&feature_schema);
  feature_index.getFeatures(docid, featuredb_txn.get(), &features);

  /* dump document */
  fnord::iputs("[document]\n  docid=$0\n\n[features]", docid);

  for (const auto& f : features) {
    auto feature_key = feature_schema.featureKey(f.first);
    fnord::iputs(
        "  $0=$1",
        feature_key.isEmpty() ? "unknown-feature": feature_key.get(),
        f.second);
  }

  /* clean up */
  featuredb_txn->abort();

  return 0;
}

